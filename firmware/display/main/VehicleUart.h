#pragma once
#include "shared/protocol/GearPacket.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace cluster::board {
// Nonblocking acquisition adapter owned by app_main, outside LVGL rendering.
// Dedicated UART1 RX on GPIO_D IO38 preserves UART0 USB console/programming.
class VehicleUart {
public:
    void initialize() {
        uart_config_t cfg{};cfg.baud_rate=115200;cfg.data_bits=UART_DATA_8_BITS;
        cfg.parity=UART_PARITY_DISABLE;cfg.stop_bits=UART_STOP_BITS_1;
        cfg.flow_ctrl=UART_HW_FLOWCTRL_DISABLE;cfg.source_clk=UART_SCLK_DEFAULT;
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_1,&cfg));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,UART_PIN_NO_CHANGE,38,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1,1024,0,8,&events_,0));
    }
    void reset() {
        ESP_ERROR_CHECK(uart_flush_input(UART_NUM_1));
        xQueueReset(events_);receiver_.reset();
    }
    void poll(std::uint64_t now) {
        // Inspect at most the driver's eight queued events each UI cycle.
        uart_event_t event{};bool fault=false;
        for(unsigned i=0;i<8&&xQueueReceive(events_,&event,0)==pdTRUE;++i)
            if(event.type==UART_FIFO_OVF||event.type==UART_BUFFER_FULL||event.type==UART_PARITY_ERR||event.type==UART_FRAME_ERR) fault=true;
        size_t waiting=0;ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1,&waiting));
        if(fault||waiting>512) {++overruns_;reset();return;}
        uint8_t bytes[512];
        const int count=uart_read_bytes(UART_NUM_1,bytes,sizeof bytes,0);
        if(count<0){++overruns_;reset();return;}
        for(int i=0;i<count;++i) if(receiver_.push(bytes[i],now))++accepted_;
    }
    ClusterState state(std::uint64_t now)const{return receiver_.state(now);}
    gearlink::ButtonClicks takeClicks(){return receiver_.takeClicks();}
    bool received()const{return receiver_.received();}
    bool fresh(std::uint64_t now)const{return receiver_.fresh(now);}
    unsigned accepted()const{return accepted_;}
    unsigned overruns()const{return overruns_;}
private:
    QueueHandle_t events_=nullptr;
    gearlink::Receiver receiver_;
    unsigned accepted_=0,overruns_=0;
};
}
