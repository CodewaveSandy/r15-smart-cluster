#include "shared/protocol/GearPacket.h"
#include "BlePhone.h"
#include "firmware/common/RecordWriter.h"
#include "shared/protocol/UartRecords.h"
#include "esp_heap_caps.h"
#include "shared/protocol/PairingIdentity.h"
#include "driver/gpio.h"
#include "InputConfig.h"
#include "BoardLed.h"
#include "shared/protocol/Button.h"
#include "shared/protocol/DiscreteInputs.h"
#include "driver/uart_vfs.h"
#include "driver/uart.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <cstdio>
#include <algorithm>

static_assert(CONFIG_ESP_MAIN_TASK_STACK_SIZE>=8192,
    "Vehicle app_main requires at least 8192 bytes: update CONFIG_ESP_MAIN_TASK_STACK_SIZE in the generated vehicle sdkconfig");

extern "C" void app_main() {
    cluster::board::disableBoardLed();
    uart_config_t link{};
    link.baud_rate=115200;link.data_bits=UART_DATA_8_BITS;
    link.parity=UART_PARITY_DISABLE;link.stop_bits=UART_STOP_BITS_1;
    link.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;link.source_clk=UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1,&link));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,wiring::displayTx,wiring::displayRx,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
    QueueHandle_t linkEvents=nullptr;
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1,4096,0,8,&linkEvents,0));
    gpio_config_t inputs{};
    const auto add=[&](wiring::Input input){if(input.pin>=0)inputs.pin_bit_mask|=1ULL<<input.pin;};
    add(wiring::tripButton);add(wiring::infoButton);
    add(wiring::left);add(wiring::right);add(wiring::highBeam);
    for(auto input:wiring::gears)add(input);
    inputs.mode=GPIO_MODE_INPUT;
    // Bench active-HIGH inputs: weak pull-downs keep unwired channels inactive.
    // Active-LOW overrides below use pull-ups instead. Neither detects wire faults.
    inputs.pull_up_en=GPIO_PULLUP_DISABLE;inputs.pull_down_en=GPIO_PULLDOWN_ENABLE;
    inputs.intr_type=GPIO_INTR_DISABLE;
    if(inputs.pin_bit_mask)ESP_ERROR_CHECK(gpio_config(&inputs));
    const auto bias=[](wiring::Input input) {
        if(input.pin>=0)ESP_ERROR_CHECK(gpio_set_pull_mode(gpio_num_t(input.pin),
            input.activeHigh?GPIO_PULLDOWN_ONLY:GPIO_PULLUP_ONLY));
    };
    bias(wiring::tripButton);bias(wiring::infoButton);
    bias(wiring::left);bias(wiring::right);bias(wiring::highBeam);
    for(auto input:wiring::gears)bias(input);
    setvbuf(stdout,nullptr,_IONBF,0);
    // Binary packet bytes must never receive stdout's default LF -> CRLF conversion.
    uart_vfs_dev_port_set_tx_line_endings(0,ESP_LINE_ENDINGS_LF);
    cluster::gearlink::Packet packet;packet.session=esp_random();if(!packet.session)packet.session=1;
    static cluster::board::BlePhone phone;phone.initialize(packet.session);
    static cluster::board::RecordWriter writer;ESP_ERROR_CHECK(writer.initialize(UART_NUM_1)?ESP_OK:ESP_FAIL);
    static cluster::uartrecords::Receiver demux;
    std::uint64_t report=0;
    unsigned rxDriverResets=0,rxBytes=0,maxRxBacklog=0,rxControls=0,displayChanges=0;
    std::uint64_t lastControlAt=0,maxControlGap=0;
    cluster::phonelink::Receiver controls;cluster::phonelink::Gate gate(packet.session);
    std::uint32_t statusSequence=0,nameSequence=0;std::uint64_t lastStatus=0,lastName=0;
    cluster::inputs::Button tripButton,infoButton;
    std::uint64_t previousSample=0;std::uint32_t maxSourceGap=0;bool sampled=false;
    for(;;) {
        const auto now=std::uint64_t(esp_timer_get_time()/1000);
        bool fault=false;uart_event_t event{};
        for(unsigned i=0;i<8&&xQueueReceive(linkEvents,&event,0)==pdTRUE;++i)
            if(event.type==UART_FIFO_OVF||event.type==UART_BUFFER_FULL||event.type==UART_FRAME_ERR||event.type==UART_PARITY_ERR)fault=true;
        size_t waiting=0;ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1,&waiting));
        maxRxBacklog=std::max(maxRxBacklog,unsigned(waiting));
        if(fault){++rxDriverResets;uart_flush_input(UART_NUM_1);xQueueReset(linkEvents);controls.reset();gate.invalidate();demux.reset();}
        else for(unsigned budget=1536;budget;){
            std::uint8_t bytes[256];const int count=uart_read_bytes(UART_NUM_1,bytes,std::min<unsigned>(sizeof bytes,budget),0);
            if(count<0){++rxDriverResets;controls.reset();gate.invalidate();break;}
            if(!count)break;
            budget-=unsigned(count);rxBytes+=unsigned(count);
            for(int i=0;i<count;++i)demux.push(bytes[i],now,
                [&](const std::uint8_t* frame,unsigned n,std::uint64_t at){
                    if(frame[3]!='P')return;
                    for(unsigned j=0;j<n;++j)if(controls.push(frame[j],at,cluster::phonelink::Kind::Control)){
                        ++rxControls;if(lastControlAt)maxControlGap=std::max(maxControlGap,at-lastControlAt);lastControlAt=at;
                        const auto snapshot=phone.snapshot(at);cluster::phonelink::Request request;bool newDisplay=false;
                        if(gate.receive(controls.latest(),at,snapshot.pairing,request,newDisplay))gate.queued(phone.post(request));
                        if(newDisplay){++displayChanges;if(snapshot.pairing)phone.post({cluster::phonelink::Action::Reject,snapshot.pairing});}
                    }
                });
        }

        const auto read=[](wiring::Input input)->cluster::Signal<bool> {
            if(input.pin<0)return {};
            return {bool(gpio_get_level(gpio_num_t(input.pin)))==input.activeHigh,cluster::Quality::Available};
        };
        packet.left=read(wiring::left);packet.right=read(wiring::right);packet.highBeam=read(wiring::highBeam);
        std::array<bool,7> asserted{};
        for(unsigned i=0;i<asserted.size();++i)asserted[i]=read(wiring::gears[i]).value;
        packet.gear=cluster::inputs::gear(asserted);
        packet.levels=std::uint8_t((wiring::left.pin>=0?gpio_get_level(gpio_num_t(wiring::left.pin)):0)|
            ((wiring::right.pin>=0?gpio_get_level(gpio_num_t(wiring::right.pin)):0)<<1));
        const auto sampledAt=std::uint64_t(esp_timer_get_time()/1000);packet.uptimeMs=std::uint32_t(sampledAt);
        if(tripButton.update(read(wiring::tripButton).value,packet.uptimeMs))++packet.tripPresses;
        if(infoButton.update(read(wiring::infoButton).value,packet.uptimeMs))++packet.infoPresses;
        if(sampled)maxSourceGap=std::max(maxSourceGap,std::uint32_t(std::min<std::uint64_t>(sampledAt-previousSample,UINT32_MAX)));
        previousSample=sampledAt;sampled=true;
        phone.poll(gate.fresh(now),packet,maxSourceGap);
        const auto bytes=cluster::gearlink::encode(packet);
        // Whole critical records enter a bounded priority queue; only its
        // worker writes UART1. No partial FIFO write or acquisition-task wait.
        writer.post(bytes.data(),bytes.size());
        // 36 bytes per 20 ms: bounded output well below the UART FIFO capacity.
        fwrite(bytes.data(),1,bytes.size(),stdout);
        if(now-lastStatus>=200) {
            lastStatus=now;const auto snapshot=phone.snapshot(now);cluster::phonelink::Packet status;
            status.vehicle=packet.session;status.sequence=++statusSequence;status.uptime=std::uint32_t(now);
            status.phone=snapshot.phone;status.pairing=snapshot.pairing;status.code=snapshot.code;gate.stamp(status);
            const auto bytes=cluster::phonelink::encode(status);
            writer.post(bytes.data(),bytes.size());
        }
        else if(now-lastName>=200) {
            lastName=now;const auto snapshot=phone.snapshot(now);
            if(snapshot.phone.available()&&snapshot.phone.connected) {
                cluster::phonemeta::Packet metadata;metadata.vehicle=packet.session;metadata.connection=snapshot.phone.connectionId;
                metadata.sequence=++nameSequence;metadata.uptime=std::uint32_t(now);metadata.data=snapshot.metadata;
                const auto bytes=cluster::phonemeta::encode(metadata);
                // 36 + 80 bytes fit the FIFO; mutually exclusive with status/name.
                writer.post(bytes.data(),bytes.size());
            } else if(snapshot.phone.available()&&snapshot.phone.pairingRequested&&snapshot.phone.displayName[0]) {
                cluster::pairidentity::Packet identity;identity.vehicle=packet.session;identity.pairing=snapshot.pairing;
                identity.sequence=++nameSequence;identity.uptime=std::uint32_t(now);identity.label=snapshot.phone.displayName;
                const auto bytes=cluster::pairidentity::encode(identity);
                // 36 telemetry + 64 identity bytes fit the FIFO; never append to a status cycle.
                writer.post(bytes.data(),bytes.size());
            }
        }
        if(now-report>=10000){
            const auto tx=writer.diagnostics();
            std::printf("@UART_RX driver_resets=%u rx_bytes=%u max_backlog=%u return_fresh=%d controls=%u display_changes=%u control_age_ms=%llu max_control_gap_ms=%llu\n",rxDriverResets,rxBytes,maxRxBacklog,int(gate.fresh(now)),rxControls,displayChanges,(unsigned long long)(lastControlAt?now-lastControlAt:0),(unsigned long long)maxControlGap);
            report=now;std::printf("@UART_TX heap_free=%u heap_min=%u largest=%u stack=%u tx_queue_full=%u tx_stack=%u sample_gap_ms=%u uart_bytes=%u write_errors=%u critical_wait_ms=%u\n",
                unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),unsigned(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)),unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)),unsigned(uxTaskGetStackHighWaterMark(nullptr)),writer.rejected(),writer.stackMargin(),unsigned(maxSourceGap),tx.bytes,tx.writeErrors,tx.maxCriticalWaitMs);
        }
        ++packet.sequence;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
