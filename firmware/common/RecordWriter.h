#pragma once
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
    unsigned rejected()const{return rejected_;}
    struct Diagnostics {unsigned bytes=0,writeErrors=0,maxCriticalWaitMs=0;};
    Diagnostics diagnostics()const{return {bytes_.load(),writeErrors_.load(),maxCriticalWaitMs_.load()};}
    unsigned stackMargin()const{return task_?unsigned(uxTaskGetStackHighWaterMark(task_)):0;}
private:
    static void run(void* p){static_cast<RecordWriter*>(p)->loop();}
    void loop(){
        QueuedRecord q;
        for(;;){
            if(xQueueReceive(queue_,&q,portMAX_DELAY)!=pdTRUE)continue;
            const unsigned wait=unsigned(xTaskGetTickCount()-q.at)*1000/configTICK_RATE_HZ;
            if(wait>maxCriticalWaitMs_)maxCriticalWaitMs_=wait;
            int written;bool flushed=true;
            if(console_){
                // Hold the console lock through flush: logs cannot split a
                // binary heartbeat or leave it waiting for a newline.
                flockfile(stdout);
                written=int(std::fwrite(q.bytes.data(),1,q.size,stdout));
                flushed=std::fflush(stdout)==0;
                funlockfile(stdout);
            }else written=uart_write_bytes(port_,q.bytes.data(),q.size);
            if(written!=int(q.size)||!flushed)++writeErrors_;
            else bytes_+=q.size;
        }
    }
    TaskHandle_t task_=nullptr;uart_port_t port_=UART_NUM_1;bool console_=false;
    std::atomic<unsigned> rejected_{0},bytes_{0},writeErrors_{0},maxCriticalWaitMs_{0};
    StaticQueue_t queueStore_{};QueueHandle_t queue_=nullptr;std::uint8_t storage_[8*sizeof(QueuedRecord)]{};
};
}
