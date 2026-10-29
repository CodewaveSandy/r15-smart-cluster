"""Exercise the actual firmware NVS adapter with fault-injected host APIs; no device."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
platform = r'''
#pragma once
#include <array>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <cstdint>
using StackType_t=uint32_t;
struct StaticTask_t {};
struct StaticQueue_t {std::array<uint8_t,8> bytes{};size_t size=0;bool full=false;};
using QueueHandle_t=StaticQueue_t*;
using TaskHandle_t=void*;
constexpr int pdTRUE=1,ESP_OK=0,NVS_READONLY=0,NVS_READWRITE=1;
constexpr unsigned portMAX_DELAY=~0u;
struct Idle {};
inline void(*worker)(void*)=nullptr;
inline void* workerContext=nullptr;
inline bool initFail=false,openFail=false,writeFail=false,commitFail=false;
inline std::vector<uint8_t> committed,pending;
inline QueueHandle_t xQueueCreateStatic(int,size_t size,uint8_t*,StaticQueue_t* q) {q->size=size;return q;}
inline int xQueueSend(QueueHandle_t q,const void* p,int) {
 if(q->full)return 0;std::memcpy(q->bytes.data(),p,q->size);q->full=true;return 1;
}
inline int xQueueOverwrite(QueueHandle_t q,const void* p) {q->full=false;return xQueueSend(q,p,0);}
inline int xQueueReceive(QueueHandle_t q,void* p,unsigned wait) {
 if(!q->full){if(wait==portMAX_DELAY)throw Idle{};return 0;}
 std::memcpy(p,q->bytes.data(),q->size);q->full=false;return 1;
}
inline TaskHandle_t xTaskCreateStatic(void(*fn)(void*),const char*,int,void* ctx,int,StackType_t*,StaticTask_t*) {
 worker=fn;workerContext=ctx;return ctx;
}
inline void drain(){try{worker(workerContext);}catch(const Idle&) {}}
using nvs_handle_t=int;
inline int nvs_flash_init(){return initFail?1:0;}
inline int nvs_open(const char*,int,nvs_handle_t* h){*h=1;return openFail?1:0;}
inline void nvs_close(nvs_handle_t){}
inline int nvs_get_blob(nvs_handle_t,const char*,void* bytes,size_t* size) {
 if(committed.empty()||committed.size()>*size)return 1;
 *size=committed.size();std::memcpy(bytes,committed.data(),*size);return 0;
}
inline int nvs_set_blob(nvs_handle_t,const char*,const void* bytes,size_t size) {
 if(writeFail)return 1;auto* p=static_cast<const uint8_t*>(bytes);pending.assign(p,p+size);return 0;
}
inline int nvs_commit(nvs_handle_t){if(commitFail)return 1;committed=pending;return 0;}
'''
test = r'''
#include "firmware/display/main/PreferenceStore.h"
#include <cassert>
int main(){
 using namespace cluster;using board::PreferenceStore;
 Preferences value;PreferenceStore store;store.initialize(value);assert(!value.light);
 assert(store.request({true}));assert(!store.request({false}));
 bool saved=false;assert(!store.result(saved));drain();assert(store.result(saved)&&saved);
 Preferences restored;PreferenceStore restart;restart.initialize(restored);assert(restored.light);
 const auto previous=committed;
 for(int failure=0;failure<3;++failure){
  openFail=failure==0;writeFail=failure==1;commitFail=failure==2;
  assert(restart.request({false}));drain();assert(restart.result(saved)&&!saved);assert(committed==previous);
 }
 openFail=writeFail=commitFail=false;
 committed[7]^=1;Preferences invalid;PreferenceStore bad;bad.initialize(invalid);assert(!invalid.light);
 committed.resize(3);Preferences shortValue;PreferenceStore shortStore;shortStore.initialize(shortValue);assert(!shortValue.light);
 initFail=true;PreferenceStore unavailable;unavailable.initialize(value);assert(!unavailable.request({true}));
}
'''
with tempfile.TemporaryDirectory(prefix="r15-nvs-check-") as directory:
    temp = Path(directory)
    (temp / "platform.h").write_text(platform)
    for name in ["freertos/FreeRTOS.h", "freertos/queue.h", "freertos/task.h", "nvs.h", "nvs_flash.h"]:
        header = temp / name
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text('#include "platform.h"\n')
    (temp / "check.cpp").write_text(test)
    subprocess.run(["c++", "-std=c++17", "-I", str(temp), "-I", str(root),
                    str(temp / "check.cpp"), "-o", str(temp / "check")], check=True)
    subprocess.run([str(temp / "check")], check=True)
print("Firmware preference adapter: restart, malformed data, queue bounds and NVS failures pass.")
