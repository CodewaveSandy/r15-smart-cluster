#pragma once
#include "shared/protocol/MapTunnel.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <cstdio>
#include <atomic>
namespace cluster::board {
// Sole binary transmitter per output. Fixed queues, no application-task waits.
// The worker may block on a single <=133 byte hardware write; critical records
// always precede the next lower-priority fragment. Console output uses fwrite's
// stream lock, so logs can appear between records but not inside a record.
class RecordWriter {
    struct QueuedRecord {maplink::WireRecord record;TickType_t at=0;};
public:
    bool initialize(uart_port_t port,bool console=false){
        port_=port;console_=console;
        high_=xQueueCreateStatic(8,sizeof(QueuedRecord),highBytes_,&highStore_);
        normal_=xQueueCreateStatic(2,sizeof(QueuedRecord),normalBytes_,&normalStore_);
        bulk_=xQueueCreateStatic(1,sizeof(QueuedRecord),bulkBytes_,&bulkStore_);
        return high_&&normal_&&bulk_&&xTaskCreate(run,"map-uart-tx",3072,this,5,&task_)==pdPASS;
    }
    bool post(const std::uint8_t* b,unsigned n,unsigned priority=0){
        if(!n||n>133)return false;
        QueuedRecord q;q.at=xTaskGetTickCount();q.record.size=n;std::copy_n(b,n,q.record.bytes.data());
        if(xQueueSend(priority==0?high_:priority==1?normal_:bulk_,&q,0)!=pdTRUE){++rejected_;return false;}return true;
    }
    unsigned rejected()const{return rejected_;}
    struct Diagnostics {unsigned bytes=0,bulkBytes=0,fragments=0,writeErrors=0,maxCriticalWaitMs=0;};
    Diagnostics diagnostics()const{return {bytes_.load(),encodedBulkBytes_.load(),fragments_.load(),writeErrors_.load(),maxCriticalWaitMs_.load()};}
    // One control plus one latest-location slot. The TX owner, not the 50 Hz
    // acquisition loop, drains their fragments. Busy includes an active sender.
    bool mapReady(bool location){portENTER_CRITICAL(&mapMux_);const bool ready=!mapBusy_[location?1:0];portEXIT_CRITICAL(&mapMux_);return ready;}
    bool postMap(const maplink::Message& m,bool location=false){
        if(!m.size||m.size>240)return false;
        const unsigned i=location?1:0;portENTER_CRITICAL(&mapMux_);
        const bool ok=!mapBusy_[i];if(ok){mapSlots_[i]=m;mapBusy_[i]=mapPending_[i]=true;}
        if(!ok)++rejected_;
        portEXIT_CRITICAL(&mapMux_);return ok;
    }
    void resetMaps(){portENTER_CRITICAL(&mapMux_);++mapEpoch_;for(unsigned i=0;i<2;++i)mapBusy_[i]=mapPending_[i]=false;portEXIT_CRITICAL(&mapMux_);}
    unsigned stackMargin()const{return task_?unsigned(uxTaskGetStackHighWaterMark(task_)):0;}
private:
    static void run(void* p){static_cast<RecordWriter*>(p)->loop();}
    void loop(){
        maplink::WireRecord r;QueuedRecord q;TickType_t refill=xTaskGetTickCount();unsigned tokens=133,remainder=0;
        maplink::Sender control,location(1);unsigned epoch=0;
        for(;;){
            const auto now=xTaskGetTickCount();const auto elapsed=now-refill;
            if(elapsed){const auto credit=std::uint64_t(elapsed)*4096+remainder;tokens=unsigned(std::min<std::uint64_t>(133,tokens+credit/configTICK_RATE_HZ));remainder=unsigned(credit%configTICK_RATE_HZ);refill=now;}
            portENTER_CRITICAL(&mapMux_);
            if(epoch!=mapEpoch_){control.reset();location.reset();epoch=mapEpoch_;}
            for(unsigned i=0;i<2;++i)if(mapPending_[i]){(i?location:control).enqueue(mapSlots_[i]);mapPending_[i]=false;}
            portEXIT_CRITICAL(&mapMux_);
            bool have=xQueueReceive(high_,&q,0)==pdTRUE;bool bulk=false,fragment=false;maplink::Sender* sent=nullptr;
            if(have){
                r=q.record;const unsigned wait=unsigned(xTaskGetTickCount()-q.at)*1000/configTICK_RATE_HZ;
                if(wait>maxCriticalWaitMs_)maxCriticalWaitMs_=wait;
            }
            if(!have&&xQueueReceive(normal_,&q,0)==pdTRUE){r=q.record;have=true;}
            if(!have){
                auto* sender=location.pending()?&location:&control;
                if(sender->pending()){
                    r=sender->next(1);bulk=sender->bulk();
                    if(!bulk||tokens>=r.size){have=fragment=true;sent=sender;if(bulk)tokens-=r.size;}
                }
            }
            if(!have&&xQueuePeek(bulk_,&q,0)==pdTRUE&&tokens>=q.record.size){have=xQueueReceive(bulk_,&q,0)==pdTRUE;if(have){r=q.record;tokens-=r.size;bulk=true;}}
            if(!have){vTaskDelay(1);continue;}
            const int written=console_?int(std::fwrite(r.bytes.data(),1,r.size,stdout)):uart_write_bytes(port_,r.bytes.data(),r.size);
            if(written!=int(r.size))++writeErrors_;
            else {bytes_+=r.size;if(bulk)encodedBulkBytes_+=r.size;if(fragment)++fragments_;}
            if(sent){
                // A failed fragment is not blindly retransmitted into a partial
                // UART record. End-to-end timeout/retry recovers the message.
                sent->sent();if(!sent->pending()){
                    portENTER_CRITICAL(&mapMux_);if(epoch==mapEpoch_)mapBusy_[sent==&location?1:0]=false;portEXIT_CRITICAL(&mapMux_);
                }
            }
        }
    }
    TaskHandle_t task_=nullptr;
    uart_port_t port_=UART_NUM_1;bool console_=false;std::atomic<unsigned> rejected_{0},bytes_{0},encodedBulkBytes_{0},fragments_{0},writeErrors_{0},maxCriticalWaitMs_{0};
    portMUX_TYPE mapMux_=portMUX_INITIALIZER_UNLOCKED;maplink::Message mapSlots_[2];
    unsigned mapEpoch_=0;bool mapBusy_[2]{},mapPending_[2]{};
    StaticQueue_t highStore_{},normalStore_{},bulkStore_{};QueueHandle_t high_=nullptr,normal_=nullptr,bulk_=nullptr;
    std::uint8_t highBytes_[8*sizeof(QueuedRecord)]{},normalBytes_[2*sizeof(QueuedRecord)]{},bulkBytes_[sizeof(QueuedRecord)]{};
};
}
