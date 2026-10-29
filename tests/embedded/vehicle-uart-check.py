"""Actual display UART adapter with fake driver queues/buffers; no serial device."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
platform = r'''
#pragma once
#include <deque>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstddef>
using std::size_t;
constexpr int UART_NUM_1=1,UART_DATA_8_BITS=8,UART_PARITY_DISABLE=0,UART_STOP_BITS_1=1;
constexpr int UART_HW_FLOWCTRL_DISABLE=0,UART_SCLK_DEFAULT=0,UART_PIN_NO_CHANGE=-1,pdTRUE=1;
enum {UART_DATA,UART_FIFO_OVF,UART_BUFFER_FULL,UART_PARITY_ERR,UART_FRAME_ERR};
struct uart_config_t {int baud_rate,data_bits,parity,stop_bits,flow_ctrl,source_clk;};
struct uart_event_t {int type;};
using QueueHandle_t=void*;
inline std::deque<uint8_t> rx;
inline std::deque<uart_event_t> events;
inline bool readError=false;
#define ESP_ERROR_CHECK(x) assert((x)==0)
inline int uart_param_config(int,const uart_config_t* c){assert(c->baud_rate==115200&&c->flow_ctrl==0);return 0;}
inline int uart_set_pin(int,int tx,int r,int,int){assert(tx==-1&&r==38);return 0;}
inline int uart_driver_install(int,int r,int tx,int count,QueueHandle_t* q,int){assert(r==1024&&tx==0&&count==8);*q=&events;return 0;}
inline int uart_flush_input(int){rx.clear();return 0;}
inline int xQueueReset(QueueHandle_t){events.clear();return 1;}
inline int xQueueReceive(QueueHandle_t,uart_event_t* e,int){if(events.empty())return 0;*e=events.front();events.pop_front();return 1;}
inline int uart_get_buffered_data_len(int,size_t* count){*count=rx.size();return 0;}
inline int uart_read_bytes(int,uint8_t* out,size_t cap,int wait){assert(wait==0&&cap==512);if(readError)return -1;int n=0;while(!rx.empty()&&size_t(n)<cap){out[n++]=rx.front();rx.pop_front();}return n;}
'''
test = r'''
#include "firmware/display/main/VehicleUart.h"
using namespace cluster;
using namespace gearlink;
void send(const Packet& p){for(auto byte:encode(p))rx.push_back(byte);}
int main(){
 board::VehicleUart link;link.initialize();link.poll(0);
 assert(!link.received()&&!link.state(0).gear.available());
 Packet p;p.session=9;p.sequence=1;p.uptimeMs=20;p.gear={2,Quality::Available};p.left={true,Quality::Available};
 send(p);link.poll(10);assert(link.accepted()==1&&link.fresh(1009));
 assert(!link.state(10).speedKph.available()&&link.state(10).turnSignalsArePhase);
 assert(link.state(1010).left.quality==Quality::Stale);
 send(p);link.poll(1010);assert(!link.fresh(1010)&&link.accepted()==1);
 ++p.sequence;p.uptimeMs+=20;p.tripPresses=1;send(p);link.poll(1100);
 assert(link.fresh(1100)&&link.takeClicks().trip==0);
 ++p.sequence;p.uptimeMs+=20;++p.tripPresses;send(p);link.poll(1120);
 assert(link.takeClicks().trip==1&&link.takeClicks().trip==0);
 auto bad=encode(p);bad[6]^=128;for(auto b:bad)rx.push_back(b);link.poll(2120);assert(!link.fresh(2120));
 rx.insert(rx.end(),513,0);link.poll(2121);assert(link.overruns()==1&&!link.received()&&rx.empty());
 ++p.session;p.sequence=0;p.uptimeMs=1;send(p);link.poll(2140);assert(link.fresh(2140)&&!link.takeClicks().trip);
 for(int fault:{UART_FIFO_OVF,UART_BUFFER_FULL,UART_PARITY_ERR,UART_FRAME_ERR}){
  events.push_back({fault});link.poll(2150);assert(!link.received());
  ++p.sequence;++p.uptimeMs;send(p);link.poll(2160);assert(link.fresh(2160));
 }
 readError=true;link.poll(2200);assert(!link.received()&&link.overruns()==6);
 readError=false;link.reset();assert(!link.fresh(2201));
}
'''
with tempfile.TemporaryDirectory(prefix="r15-uart-check-") as directory:
    temp = Path(directory)
    (temp / "platform.h").write_text(platform)
    for name in ["driver/uart.h", "esp_err.h", "freertos/FreeRTOS.h", "freertos/queue.h"]:
        header = temp / name
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text('#include "platform.h"\n')
    (temp / "check.cpp").write_text(test)
    subprocess.run(["c++", "-std=c++17", "-I", str(temp), "-I", str(root),
                    str(temp / "check.cpp"), "-o", str(temp / "check")], check=True)
    subprocess.run([str(temp / "check")], check=True)
print("Display UART adapter: receive, stale, duplicate, CRC, clicks, overflow/errors and reset pass.")
