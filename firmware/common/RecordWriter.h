#pragma once
#include "shared/protocol/MapTunnel.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <cstdio>
namespace cluster::board {
// Sole binary transmitter per output. Fixed queues, no application-task waits.
// The worker may block on a single <=133 byte hardware write; critical records
// always precede the next lower-priority fragment. Console output uses fwrite's
// stream lock, so logs can appear between records but not inside a record.
class RecordWriter {
public:
    bool initialize(uart_port_t port,bool console=false){
        port_=port;console_=console;
        high_=xQueueCreateStatic(8,sizeof(maplink::WireRecord),highBytes_,&highStore_);
        normal_=xQueueCreateStatic(2,sizeof(maplink::WireRecord),normalBytes_,&normalStore_);
        bulk_=xQueueCreateStatic(1,sizeof(maplink::WireRecord),bulkBytes_,&bulkStore_);
        return high_&&normal_&&bulk_&&xTaskCreate(run,"map-uart-tx",3072,this,5,&task_)==pdPASS;
    }
    bool post(const std::uint8_t* b,unsigned n,unsigned priority=0){
        if(!n||n>133)return false;
        maplink::WireRecord r;r.size=n;std::copy_n(b,n,r.bytes.data());
        if(xQueueSend(priority==0?high_:priority==1?normal_:bulk_,&r,0)!=pdTRUE){++rejected_;return false;}return true;
    }
    unsigned rejected()const{return rejected_;}
    unsigned stackMargin()const{return task_?unsigned(uxTaskGetStackHighWaterMark(task_)):0;}
private:
    static void run(void* p){static_cast<RecordWriter*>(p)->loop();}
    void loop(){
        maplink::WireRecord r;TickType_t refill=xTaskGetTickCount();unsigned tokens=133;
        for(;;){
            const auto now=xTaskGetTickCount();const auto elapsed=now-refill;
            if(elapsed){tokens=std::min(133u,tokens+unsigned(elapsed)*4096/configTICK_RATE_HZ);refill=now;}
            bool have=xQueueReceive(high_,&r,0)==pdTRUE;
            if(!have)have=xQueueReceive(normal_,&r,0)==pdTRUE;
            if(!have&&xQueuePeek(bulk_,&r,0)==pdTRUE&&tokens>=r.size){have=xQueueReceive(bulk_,&r,0)==pdTRUE;if(have)tokens-=r.size;}
            if(!have){vTaskDelay(1);continue;}
            if(console_)std::fwrite(r.bytes.data(),1,r.size,stdout);
            else uart_write_bytes(port_,r.bytes.data(),r.size);
        }
    }
    TaskHandle_t task_=nullptr;
    uart_port_t port_=UART_NUM_1;bool console_=false;unsigned rejected_=0;
    StaticQueue_t highStore_{},normalStore_{},bulkStore_{};QueueHandle_t high_=nullptr,normal_=nullptr,bulk_=nullptr;
    std::uint8_t highBytes_[8*sizeof(maplink::WireRecord)]{},normalBytes_[2*sizeof(maplink::WireRecord)]{},bulkBytes_[sizeof(maplink::WireRecord)]{};
};
}
