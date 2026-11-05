#pragma once
#include "shared/protocol/GearPacket.h"
#include "shared/protocol/UartRecords.h"
#include "shared/application/TurnGuidanceStore.h"
#include "firmware/common/RecordWriter.h"
#include "shared/protocol/PhoneLink.h"
#include "shared/protocol/PairingIdentity.h"
#include "shared/protocol/PhoneMetadata.h"
#include "embedded/application/SettingsNavigation.h"
#include <cstdio>
#include "driver/uart.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace cluster::board {
// Nonblocking acquisition adapter owned by app_main, outside LVGL rendering.
// Dedicated UART1 RX on GPIO_D IO38 preserves UART0 USB console/programming.
class VehicleUart {
public:
    void initialize(std::uint32_t session=1) {
        phone_.initialize(session);guidance_.initialize(session);
        static_assert(sizeof(guidance::Store)+sizeof(RecordWriter)+sizeof(uartrecords::Receiver)+sizeof(uartrecords::Assembler)+sizeof(guidance::TurnGuidanceState)+2*sizeof(guidance::Message)<4096,"Display fixed guidance transport budget");
        ESP_ERROR_CHECK(writer_.initialize(UART_NUM_0,true)?ESP_OK:ESP_FAIL);
        uart_config_t cfg{};cfg.baud_rate=115200;cfg.data_bits=UART_DATA_8_BITS;
        cfg.parity=UART_PARITY_DISABLE;cfg.stop_bits=UART_STOP_BITS_1;
        cfg.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;cfg.source_clk=UART_SCLK_DEFAULT;
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_1,&cfg));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,UART_PIN_NO_CHANGE,38,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1,4096,0,8,&events_,0));
    }
    void reset() {
        ESP_ERROR_CHECK(uart_flush_input(UART_NUM_1));
        xQueueReset(events_);receiver_.reset();status_.reset();identity_.reset();metadata_.reset();phone_.reset();demux_.reset();assembly_.reset();guidance_.invalidate(lastPoll_);writer_.resetFrames();
    }
    void poll(std::uint64_t now) {
        lastPoll_=now;
        // Inspect at most the driver's eight queued events each UI cycle.
        uart_event_t event{};bool fault=false;
        for(unsigned i=0;i<8&&xQueueReceive(events_,&event,0)==pdTRUE;++i)
            if(event.type==UART_FIFO_OVF||event.type==UART_BUFFER_FULL||event.type==UART_PARITY_ERR||event.type==UART_FRAME_ERR) fault=true;
        size_t waiting=0;ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1,&waiting));
        maxBacklog_=std::max(maxBacklog_,unsigned(waiting));
        if(fault) {++overruns_;reset();return;}
        // Drain a bounded burst before returning to LVGL. A high-water mark
        // alone is not data corruption and must not erase valid heartbeats.
        for(unsigned budget=1536;budget;){
        uint8_t bytes[512];
        const int count=uart_read_bytes(UART_NUM_1,bytes,std::min<unsigned>(sizeof bytes,budget),0);
        if(count<0){++overruns_;reset();return;}
        if(!count)break;
        budget-=unsigned(count);receivedBytes_+=unsigned(count);
        for(int i=0;i<count;++i)demux_.push(bytes[i],now,
            [&](const std::uint8_t* frame,unsigned n,std::uint64_t at){
                for(unsigned j=0;j<n;++j){
                    if(frame[3]=='N')identity_.push(frame[j],at);
                    else if(frame[3]=='M')metadata_.push(frame[j],at);
                    else if(frame[3]=='G'){if(receiver_.push(frame[j],at))++accepted_;}
                    else if(frame[3]=='P'&&status_.push(frame[j],at,phonelink::Kind::Status)){
                        ++statusAccepted_;if(lastStatusAt_)maxStatusGap_=std::max(maxStatusGap_,at-lastStatusAt_);lastStatusAt_=at;
                        phone_.update(status_.latest(),at);
                    }
                }
            },[&](const std::uint8_t* frame,unsigned n,std::uint64_t at){
                assembly_.push(frame,n,at,1,[&](const std::uint8_t* b,unsigned size,std::uint64_t){
                    const auto actual=std::uint64_t(esp_timer_get_time()/1000);guidanceSession(actual);guidance::Message m;m.size=size;std::copy_n(b,size,m.bytes.begin());
                    if(size>=4&&guidance::magic(b,'U'))guidance_.consume(m,actual);
                });
            });
        }
        guidanceSession(now);guidance_.poll(now);
        phonelink::Packet control;
        if(phone_.outgoing(now,control)) {
            const auto bytes=phonelink::encode(control);
            writer_.post(bytes.data(),bytes.size());
        }
        if(now-lastCapability_>=1000){const auto b=guidance_.capability(now);writer_.postFramed(b.data(),b.size(),2);lastCapability_=now;}
        else if(now-lastGuidanceStatus_>=500){sendGuidanceStatus(now);}
    }
    guidance::TurnGuidanceState guidanceState(std::uint64_t now)const{return guidance_.view(now);}
    void guidancePublished(std::uint64_t now,bool visible){
        const auto before=guidance_.status(now);guidance_.published(now,visible);
        if(!(before[7]&4)||bool(before[7]&128)!=visible)sendGuidanceStatus(now);
    }
    guidance::Store::Diagnostics guidanceDiagnostics()const{return guidance_.diagnostics();}
    const uartrecords::Receiver::Diagnostics& framingDiagnostics()const{return demux_.diagnostics();}
    unsigned txStackMargin()const{return writer_.stackMargin();}
    unsigned txRejected()const{return writer_.rejected();}
    RecordWriter::Diagnostics txDiagnostics()const{return writer_.diagnostics();}
    ClusterState state(std::uint64_t now)const {
        auto state=receiver_.state(now);state.phone=phone_.phone(now,receiver_.latest().session);
        if(state.phone.available()&&state.phone.pairingRequested)state.phone.displayName=identity_.label(now,receiver_.latest().session,status_.latest().pairing);
        if(state.phone.available()&&state.phone.connected)if(const auto* p=metadata_.current(now,receiver_.latest().session,state.phone.connectionId)) {
            state.phone.displayName=p->data.label;state.phone.batteryPercent=p->data.battery;
            if(p->data.battery>=0)state.phoneBatteryPercent={p->data.battery,Quality::Available};
        }
        state.phoneConnected={state.phone.connected,state.phone.quality};return state;
    }
    bool synchronizeClock(Clock& clock,std::uint64_t now) {
        const auto phone=phone_.phone(now,receiver_.latest().session);
        if(!phone.available()||!phone.connected)return false;
        const auto* p=metadata_.current(now,receiver_.latest().session,phone.connectionId);
        if(!p||!p->data.epoch||(clockVehicle_==p->vehicle&&clockSequence_==p->sequence))return false;
        clockVehicle_=p->vehicle;clockSequence_=p->sequence;return clock.synchronize(p->data.epoch,p->data.offset,now);
    }
    bool command(SettingsEffect effect,std::uint64_t now) {
        phonelink::Action action;
        switch(effect){
        case SettingsEffect::PhonePair:action=phonelink::Action::Pair;break;
        case SettingsEffect::PhoneApprove:action=phonelink::Action::Approve;break;
        case SettingsEffect::PhoneReject:action=phonelink::Action::Reject;break;
        case SettingsEffect::PhoneDisconnect:action=phonelink::Action::Disconnect;break;
        case SettingsEffect::PhoneForget:action=phonelink::Action::Forget;break;
        default:return false;
        }
        return phone_.post(action,now,receiver_.latest().session);
    }
    gearlink::ButtonClicks takeClicks(){return receiver_.takeClicks();}
    bool received()const{return receiver_.received();}
    bool fresh(std::uint64_t now)const{return receiver_.fresh(now);}
    unsigned accepted()const{return accepted_;}
    unsigned overruns()const{return overruns_;}
    unsigned rxBytes()const{return receivedBytes_;}
    unsigned maxBacklog()const{return maxBacklog_;}
    unsigned statusAccepted()const{return statusAccepted_;}
    std::uint64_t statusAge(std::uint64_t now)const{return lastStatusAt_&&now>=lastStatusAt_?now-lastStatusAt_:0;}
    std::uint64_t maxStatusGap()const{return maxStatusGap_;}
private:
    void guidanceSession(std::uint64_t now){
        const auto p=phone_.phone(now,receiver_.latest().session);
        guidance_.session(receiver_.latest().session,p.connectionId,receiver_.fresh(now)&&p.available()&&p.connected,now);
    }
    void sendGuidanceStatus(std::uint64_t now){const auto b=guidance_.status(now);writer_.postFramed(b.data(),b.size(),2);lastGuidanceStatus_=now;}
    guidance::Store guidance_;uartrecords::Assembler assembly_;std::uint64_t lastCapability_=0,lastGuidanceStatus_=0,lastPoll_=0;
    RecordWriter writer_;uartrecords::Receiver demux_;
    QueueHandle_t events_=nullptr;
    gearlink::Receiver receiver_;
    pairidentity::Receiver identity_;phonemeta::Receiver metadata_;
    std::uint32_t clockVehicle_=0,clockSequence_=0;
    phonelink::Receiver status_;phonelink::Client phone_;
    unsigned accepted_=0,overruns_=0,receivedBytes_=0,maxBacklog_=0,statusAccepted_=0;
    std::uint64_t lastStatusAt_=0,maxStatusGap_=0;
};
}
