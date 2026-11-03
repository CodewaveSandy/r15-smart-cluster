#include "shared/protocol/GearPacket.h"
#include "BlePhone.h"
#include "firmware/common/RecordWriter.h"
#include "esp_heap_caps.h"
#include "shared/protocol/PairingIdentity.h"
#include "driver/gpio.h"
#include "InputConfig.h"
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

extern "C" void app_main() {
    uart_config_t link{};
    link.baud_rate=115200;link.data_bits=UART_DATA_8_BITS;
    link.parity=UART_PARITY_DISABLE;link.stop_bits=UART_STOP_BITS_1;
    link.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;link.source_clk=UART_SCLK_DEFAULT;
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1,&link));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,wiring::displayTx,wiring::displayRx,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
    QueueHandle_t linkEvents=nullptr;
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1,2048,0,8,&linkEvents,0));
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
    static cluster::maplink::Demux demux;static cluster::maplink::Assembler assembly;
    static cluster::maplink::Sender mapSender,locationSender(1);
    std::uint32_t mapConnection=0,mapDisplay=0;std::uint64_t report=0;
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
        if(fault||waiting>1536){uart_flush_input(UART_NUM_1);xQueueReset(linkEvents);controls.reset();gate.invalidate();demux.reset();assembly.reset();phone.invalidateMap();mapSender.reset();locationSender.reset();}
        else {
            std::uint8_t bytes[256];const int count=uart_read_bytes(UART_NUM_1,bytes,sizeof bytes,0);
            if(count<0){controls.reset();gate.invalidate();}
            for(int i=0;i<count;++i)demux.push(bytes[i],now,
                [&](const std::uint8_t* frame,unsigned n,std::uint64_t at){
                    if(frame[3]!='P')return;
                    for(unsigned j=0;j<n;++j)if(controls.push(frame[j],at,cluster::phonelink::Kind::Control)){
                        const auto snapshot=phone.snapshot(at);cluster::phonelink::Request request;bool newDisplay=false;
                        if(gate.receive(controls.latest(),at,snapshot.pairing,request,newDisplay))gate.queued(phone.post(request));
                        if(newDisplay){phone.invalidateMap();mapSender.reset();locationSender.reset();if(snapshot.pairing)phone.post({cluster::phonelink::Action::Reject,snapshot.pairing});}
                    }
                },[&](const std::uint8_t* frame,unsigned n,std::uint64_t at){
                    cluster::maplink::Message message;if(assembly.push(frame,n,at,2,message)){
                        if(cluster::maplink::validCapability(message.bytes.data(),message.size)&&cluster::maplink::get(message.bytes.data(),8)==packet.session){
                            const auto display=std::uint32_t(cluster::maplink::get(message.bytes.data(),16));
                            if(display!=mapDisplay){mapSender.reset();locationSender.reset();mapDisplay=display;}
                        }
                        phone.mapDownstream(message,at);
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
        const auto mapPhone=phone.snapshot(now);
        const auto connection=mapPhone.phone.available()&&mapPhone.phone.connected&&gate.fresh(now)?mapPhone.phone.connectionId:0;
        if(connection!=mapConnection){mapSender.reset();locationSender.reset();mapConnection=connection;}
        if(connection){
            cluster::maplink::Message m;
            const auto eligible=[&](const cluster::maplink::Message& q){return cluster::maplink::get(q.bytes.data(),8)==packet.session&&cluster::maplink::get(q.bytes.data(),12)==connection&&cluster::maplink::get(q.bytes.data(),16)==mapDisplay;};
            if(phone.takeMap(m,false,true)&&eligible(m)){mapSender.reset();locationSender.reset();mapSender.enqueue(m);}
            if(!mapSender.pending()&&phone.takeMap(m,false)&&eligible(m))mapSender.enqueue(m);
            if(!locationSender.pending()&&phone.takeMap(m,true)&&eligible(m))locationSender.enqueue(m);
            // One fragment per sample cycle; latest fix precedes bulk map.
            auto* sender=locationSender.pending()?&locationSender:&mapSender;
            if(sender->pending()){
                const auto r=sender->next(1);if(writer.post(r.bytes.data(),r.size,sender->bulk()?2:1))sender->sent();
            }
        }
        if(now-report>=10000){
            report=now;std::printf("@MAP heap_free=%u heap_min=%u largest=%u stack=%u tx_queue_full=%u tx_stack=%u sample_gap_ms=%u\n",
                unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),unsigned(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)),unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)),unsigned(uxTaskGetStackHighWaterMark(nullptr)),writer.rejected(),writer.stackMargin(),unsigned(maxSourceGap));
        }
        ++packet.sequence;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
