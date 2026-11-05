#pragma once
#include "shared/protocol/FramedUart.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
namespace cluster::board {
// Sole binary transmitter per output. Fixed queue; acquisition never waits on TX.
class RecordWriter {
    struct QueuedRecord {std::array<std::uint8_t,80> bytes{};unsigned size=0;TickType_t at=0;};
public:
    bool initialize(uart_port_t port,bool console=false){
        port_=port;console_=console;
        queue_=xQueueCreateStatic(8,sizeof(QueuedRecord),storage_,&queueStore_);
        return queue_&&xTaskCreate(run,"record-uart-tx",3072,this,5,&task_)==pdPASS;
    }
    bool post(const std::uint8_t* b,unsigned n){
        if(!n||n>80)return false;
        QueuedRecord q;q.at=xTaskGetTickCount();q.size=n;std::copy_n(b,n,q.bytes.data());
        if(xQueueSend(queue_,&q,0)!=pdTRUE){++rejected_;return false;}return true;
    }
    // One replaceable framed message. Clear/reset preempts at a whole-record
    // boundary; legacy queue always wins before the next guidance fragment.
    bool postFramed(const std::uint8_t* b,unsigned n,unsigned direction,bool preempt=false){
        if(!n||n>224||direction<1||direction>2)return false;
        portENTER_CRITICAL(&frameMux_);
        if(preempt)++epoch_;
        pending_.size=n;std::copy_n(b,n,pending_.bytes.begin());direction_=direction;pendingReady_=true;
        portEXIT_CRITICAL(&frameMux_);return true;
    }
    void resetFrames(){portENTER_CRITICAL(&frameMux_);++epoch_;pendingReady_=false;portEXIT_CRITICAL(&frameMux_);}
    unsigned rejected()const{return rejected_;}
    struct Diagnostics {unsigned bytes=0,writeErrors=0,maxCriticalWaitMs=0,fragments=0,framedBytes=0;};
    Diagnostics diagnostics()const{return {bytes_.load(),writeErrors_.load(),maxCriticalWaitMs_.load(),fragments_.load(),framedBytes_.load()};}
    unsigned stackMargin()const{return task_?unsigned(uxTaskGetStackHighWaterMark(task_)):0;}
private:
    static void run(void* p){static_cast<RecordWriter*>(p)->loop();}
    void loop(){
        QueuedRecord q;uartrecords::Message active;unsigned offset=0,sequence=0,epoch=0,direction=1;
        TickType_t refill=xTaskGetTickCount();unsigned tokens=133,remainder=0;
        for(;;){
            const auto tick=xTaskGetTickCount(),elapsed=tick-refill;
            if(elapsed){const auto credit=std::uint64_t(elapsed)*1536+remainder;tokens=unsigned(std::min<std::uint64_t>(133,tokens+credit/configTICK_RATE_HZ));remainder=unsigned(credit%configTICK_RATE_HZ);refill=tick;}
            portENTER_CRITICAL(&frameMux_);
            if(epoch!=epoch_){active.size=0;offset=0;epoch=epoch_;}
            if(!active.size&&pendingReady_){active=pending_;direction=direction_;pendingReady_=false;offset=0;sequence=(sequence+1)&65535;}
            portEXIT_CRITICAL(&frameMux_);
            uartrecords::WireRecord frame;bool framed=false;
            const bool critical=xQueueReceive(queue_,&q,0)==pdTRUE;
            if(critical){
                const unsigned wait=unsigned(xTaskGetTickCount()-q.at)*1000/configTICK_RATE_HZ;
                if(wait>maxCriticalWaitMs_)maxCriticalWaitMs_=wait;
            }else if(active.size){
                frame=uartrecords::fragment(active.bytes.data(),active.size,direction,sequence,offset);
                if(tokens<frame.size){vTaskDelay(1);continue;}
                tokens-=frame.size;framed=true;
            }else {vTaskDelay(1);continue;}
            const auto* data=framed?frame.bytes.data():q.bytes.data();const unsigned size=framed?frame.size:q.size;
            int written;bool flushed=true;
            if(console_){
                // Hold the console lock through flush: logs cannot split a
                // binary heartbeat or leave it waiting for a newline.
                flockfile(stdout);
                written=int(std::fwrite(data,1,size,stdout));
                flushed=std::fflush(stdout)==0;
                funlockfile(stdout);
            }else written=uart_write_bytes(port_,data,size);
            if(written!=int(size)||!flushed)++writeErrors_;
            else {bytes_+=size;if(framed){++fragments_;framedBytes_+=size;}}
            if(framed){offset+=std::min(48u,active.size-offset);if(offset==active.size)active.size=0;}
        }
    }
    TaskHandle_t task_=nullptr;uart_port_t port_=UART_NUM_1;bool console_=false;
    std::atomic<unsigned> rejected_{0},bytes_{0},writeErrors_{0},maxCriticalWaitMs_{0},fragments_{0},framedBytes_{0};
    portMUX_TYPE frameMux_=portMUX_INITIALIZER_UNLOCKED;uartrecords::Message pending_;unsigned epoch_=0,direction_=1;bool pendingReady_=false;
    StaticQueue_t queueStore_{};QueueHandle_t queue_=nullptr;std::uint8_t storage_[8*sizeof(QueuedRecord)]{};
};
}
