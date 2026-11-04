#pragma once
#include "shared/protocol/GearPacket.h"
#include "MapMemory.h"
#include "firmware/common/RecordWriter.h"
#include "shared/protocol/PhoneLink.h"
#include "shared/protocol/PairingIdentity.h"
#include "shared/protocol/PhoneMetadata.h"
#include "embedded/application/SettingsNavigation.h"
#include <cstdio>
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace cluster::board {
// Nonblocking acquisition adapter owned by app_main, outside LVGL rendering.
// Dedicated UART1 RX on GPIO_D IO38 preserves UART0 USB console/programming.
class VehicleUart {
public:
    void initialize(std::uint32_t session=1) {
        phone_.initialize(session);
        reserveMaps(maps_,session);
        reserveNavigation(navigation_,maps_,session);
        ESP_ERROR_CHECK(writer_.initialize(UART_NUM_0,true)?ESP_OK:ESP_FAIL);
        uart_config_t cfg{};cfg.baud_rate=115200;cfg.data_bits=UART_DATA_8_BITS;
        cfg.parity=UART_PARITY_DISABLE;cfg.stop_bits=UART_STOP_BITS_1;
        cfg.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;cfg.source_clk=UART_SCLK_DEFAULT;
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_1,&cfg));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,UART_PIN_NO_CHANGE,38,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1,2048,0,8,&events_,0));
    }
    void reset() {
        ESP_ERROR_CHECK(uart_flush_input(UART_NUM_1));
        xQueueReset(events_);receiver_.reset();status_.reset();identity_.reset();metadata_.reset();phone_.reset();demux_.reset();assembly_.reset();maps_.session(0,0,false);navigation_.session(0,0,false);
    }
    void poll(std::uint64_t now) {
        // Inspect at most the driver's eight queued events each UI cycle.
        uart_event_t event{};bool fault=false;
        for(unsigned i=0;i<8&&xQueueReceive(events_,&event,0)==pdTRUE;++i)
            if(event.type==UART_FIFO_OVF||event.type==UART_BUFFER_FULL||event.type==UART_PARITY_ERR||event.type==UART_FRAME_ERR) fault=true;
        size_t waiting=0;ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1,&waiting));
        if(fault||waiting>1536) {++overruns_;reset();return;}
        uint8_t bytes[512];
        const int count=uart_read_bytes(UART_NUM_1,bytes,sizeof bytes,0);
        if(count<0){++overruns_;reset();return;}
        for(int i=0;i<count;++i)demux_.push(bytes[i],now,
            [&](const std::uint8_t* frame,unsigned n,std::uint64_t at){
                for(unsigned j=0;j<n;++j){
                    if(frame[3]=='N')identity_.push(frame[j],at);
                    else if(frame[3]=='M')metadata_.push(frame[j],at);
                    else if(frame[3]=='G'){if(receiver_.push(frame[j],at))++accepted_;}
                    else if(frame[3]=='P'&&status_.push(frame[j],at,phonelink::Kind::Status))phone_.update(status_.latest(),at);
                }
                synchronizeMapSession(at);
            },[&](const std::uint8_t* frame,unsigned n,std::uint64_t at){
                maplink::Message message;if(assembly_.push(frame,n,at,1,message)){
                    synchronizeMapSession(at);
                    if(navlink::command(message)){
                        navigation_.command(message,at);
                        if(message.bytes[5]==1&&navigation_.active()){if(!navigationMode_)maps_.discard();navigationMode_=true;interestPending_=routeNeedPending_=false;}
                    }else if(maplink::validCommand(message)){
                        const auto phone=phone_.phone(at,receiver_.latest().session);
                        const bool context=receiver_.fresh(at)&&phone.available()&&phone.connected&&maplink::get(message.bytes.data(),8)==receiver_.latest().session&&maplink::get(message.bytes.data(),12)==phone.connectionId&&maplink::get(message.bytes.data(),16)==maplink::get(maps_.capability().data(),16);
                        if(navigationMode_&&context&&(message.bytes[5]==1||message.bytes[5]==3)){navigation_.close();maps_.discard();navigationMode_=false;}
                        if(!navigationMode_)maps_.command(message,at);
                    }
                }
            });
        synchronizeMapSession(now);if(navigationMode_)navigation_.poll(now);else maps_.poll(now);
        phonelink::Packet control;
        if(phone_.outgoing(now,control)) {
            const auto bytes=phonelink::encode(control);
            writer_.post(bytes.data(),bytes.size());
        }
        // Coalesced end-to-end status; heartbeat capability is independent of
        // whether the phone is subscribed or currently sending a map.
        if(maps_.takeDirty()||now-lastMapStatus_>=1000)statusPending_=true;
        if(navigation_.takeDirty()||now-lastNavStatus_>=500)navStatusPending_=true;
        if(navigation_.takeInterest())interestPending_=true;
        if(navigation_.takeRouteNeed())routeNeedPending_=true;
        if(!navigation_.active())interestPending_=routeNeedPending_=false;
        if(!sender_.pending()){
            maplink::Message m;
            if(now-lastCapability_>=1000){const auto b=maps_.capability();m.size=b.size();std::copy(b.begin(),b.end(),m.bytes.begin());lastCapability_=now;}
            else if(now-lastNavCapability_>=1000){const auto b=navigation_.capability();m.size=b.size();std::copy(b.begin(),b.end(),m.bytes.begin());lastNavCapability_=now;}
            else if(navStatusPending_){m=navigation_.status(now);navStatusPending_=false;lastNavStatus_=now;}
            else if(statusPending_){const auto b=maps_.status(now);m.size=b.size();std::copy(b.begin(),b.end(),m.bytes.begin());statusPending_=false;lastMapStatus_=now;}
            else if(routeNeedPending_){m=navigation_.routeNeed();routeNeedPending_=false;}
            else if(interestPending_&&now-lastInterest_>=250){m=navigation_.interest();interestPending_=false;lastInterest_=now;}
            if(m.size)sender_.enqueue(m);
        }
        if(sender_.pending()){
            const auto r=sender_.next(2);if(writer_.post(r.bytes.data(),r.size,1))sender_.sent();
        }
    }
    maplink::MapStore& mapStore(){return maps_;}
    navlink::NavigationStore& navigation(){return navigation_;}
    bool navigationMode()const{return navigationMode_;}
    void navigationCamera(const maps::MapCamera& camera,std::uint64_t now){if(navigationMode_)navigation_.camera(camera,now);}
    unsigned txStackMargin()const{return writer_.stackMargin();}
    unsigned txRejected()const{return writer_.rejected();}
    RecordWriter::Diagnostics txDiagnostics()const{return writer_.diagnostics();}
    const maplink::Demux::Diagnostics& rxDiagnostics()const{return demux_.diagnostics();}
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
private:
    void synchronizeMapSession(std::uint64_t now){
        const auto p=phone_.phone(now,receiver_.latest().session);
        maps_.session(receiver_.latest().session,p.connectionId,receiver_.fresh(now)&&p.available()&&p.connected);
        navigation_.session(receiver_.latest().session,p.connectionId,receiver_.fresh(now)&&p.available()&&p.connected);
    }
    maplink::MapStore maps_;RecordWriter writer_;maplink::Demux demux_;maplink::Assembler assembly_;maplink::Sender sender_;
    navlink::NavigationStore navigation_;bool navigationMode_=false,navStatusPending_=true,interestPending_=false,routeNeedPending_=false;
    std::uint64_t lastNavCapability_=0,lastNavStatus_=0,lastInterest_=0;
    std::uint64_t lastCapability_=0,lastMapStatus_=0;bool statusPending_=true;
    QueueHandle_t events_=nullptr;
    gearlink::Receiver receiver_;
    pairidentity::Receiver identity_;phonemeta::Receiver metadata_;
    std::uint32_t clockVehicle_=0,clockSequence_=0;
    phonelink::Receiver status_;phonelink::Client phone_;
    unsigned accepted_=0,overruns_=0;
};
}
