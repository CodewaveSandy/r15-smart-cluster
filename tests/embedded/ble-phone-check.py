"""Exercise the actual NimBLE adapter with host API fakes; no radio or NVS device."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
platform = r'''
#pragma once
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
#include <cassert>
#define CONFIG_BT_NIMBLE_SM_LVL 3
#define CONFIG_BT_NIMBLE_SM_SC_ONLY 1
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
constexpr int pdTRUE=1,ESP_OK=0,BLE_HS_ENOENT=2;
constexpr uint16_t BLE_HS_CONN_HANDLE_NONE=65535;
constexpr int BLE_SM_IO_CAP_DISP_YES_NO=1,BLE_SM_PAIR_KEY_DIST_ENC=1,BLE_SM_PAIR_KEY_DIST_ID=2,BLE_SM_IOACT_NUMCMP=4;
constexpr int BLE_HS_ADV_F_DISC_GEN=1,BLE_HS_ADV_F_BREDR_UNSUP=2,BLE_GAP_CONN_MODE_UND=1,BLE_GAP_DISC_MODE_GEN=1,BLE_HS_FOREVER=-1,BLE_ERR_REM_USER_CONN_TERM=19;
constexpr int BLE_GAP_EVENT_CONNECT=1,BLE_GAP_EVENT_DISCONNECT=2,BLE_GAP_EVENT_PASSKEY_ACTION=3,BLE_GAP_EVENT_ENC_CHANGE=4,BLE_GAP_EVENT_REPEAT_PAIRING=5,BLE_GAP_EVENT_ADV_COMPLETE=6,BLE_GAP_REPEAT_PAIRING_IGNORE=0;
struct StaticQueue_t {unsigned size=0,cap=0;std::deque<std::vector<uint8_t>> items;};
using QueueHandle_t=StaticQueue_t*;
inline QueueHandle_t xQueueCreateStatic(unsigned cap,unsigned size,uint8_t*,StaticQueue_t* queue){queue->size=size;queue->cap=cap;return queue;}
inline int xQueueSend(QueueHandle_t q,const void* value,int){if(q->items.size()>=q->cap)return 0;auto* p=static_cast<const uint8_t*>(value);q->items.emplace_back(p,p+q->size);return 1;}
inline int xQueueReceive(QueueHandle_t q,void* out,int){if(q->items.empty())return 0;std::memcpy(out,q->items.front().data(),q->size);q->items.pop_front();return 1;}
inline uint64_t millis=100;
inline int64_t esp_timer_get_time(){return int64_t(millis*1000);}
inline uint32_t esp_random(){return 987;}
inline bool nvsFail=false,storeFail=false;
inline int nvs_flash_init(){return nvsFail?1:0;}
union ble_store_value {int value;};union ble_store_key {int key;};
using ble_store_write_fn=int(int,const union ble_store_value*);
using ble_store_delete_fn=int(int,const union ble_store_key*);
inline int defaultWrite(int,const union ble_store_value*){return storeFail?1:0;}
inline int defaultDelete(int,const union ble_store_key*){return storeFail?1:0;}
struct Config {
 void(*reset_cb)(int)=nullptr;void(*sync_cb)()=nullptr;
 int sm_io_cap=0,sm_bonding=0,sm_mitm=0,sm_sc=0,sm_our_key_dist=0,sm_their_key_dist=0;
 ble_store_write_fn* store_write_cb=defaultWrite;ble_store_delete_fn* store_delete_cb=defaultDelete;
};
inline Config ble_hs_cfg;
struct ble_addr_t {uint8_t type=0,val[6]{};};
struct ble_gap_sec_state {bool encrypted=false,authenticated=false,bonded=false;unsigned key_size=0;};
struct ble_gap_conn_desc {ble_gap_sec_state sec_state;ble_addr_t peer_id_addr;};
struct ble_gap_event {
 int type=0;struct {int status=0;uint16_t conn_handle=7;}connect,enc_change;
 struct {uint16_t conn_handle=7;struct {int action=BLE_SM_IOACT_NUMCMP;uint32_t numcmp=123456;}params;}passkey;
};
struct ble_sm_io {uint8_t action=0,numcmp_accept=0;};
inline ble_gap_conn_desc descriptor;inline ble_addr_t storedAddress;
inline bool advertising=false;inline int bonds=0,terminated=0,accepted=0,rejected=0;
inline int(*gapCallback)(ble_gap_event*,void*)=nullptr;inline void* gapContext=nullptr;
struct ble_hs_adv_fields {int flags=0;const uint8_t* name=nullptr;unsigned name_len=0;int name_is_complete=0;};
struct ble_gap_adv_params {int conn_mode=0,disc_mode=0;};
inline const char* deviceName="";
inline int ble_svc_gap_device_name_set(const char* name){deviceName=name;return 0;}
inline const char* ble_svc_gap_device_name(){return deviceName;}
inline void ble_svc_gap_init(){}inline void ble_svc_gatt_init(){}
inline int ble_hs_id_infer_auto(int,uint8_t* type){*type=0;return 0;}
inline int ble_store_util_bonded_peers(ble_addr_t* address,int* count,int max){assert(max==1);*count=bonds;*address=storedAddress;return 0;}
inline int ble_gap_adv_active(){return advertising;}
inline int ble_gap_adv_set_fields(const ble_hs_adv_fields* fields){assert(fields->name_len==11);return 0;}
inline int ble_gap_adv_start(int,void*,int,ble_gap_adv_params*,int(*callback)(ble_gap_event*,void*),void* context){advertising=true;gapCallback=callback;gapContext=context;return 0;}
inline int ble_gap_conn_find(uint16_t,ble_gap_conn_desc* out){*out=descriptor;return 0;}
inline int ble_gap_security_initiate(uint16_t){return 0;}
inline int ble_gap_terminate(uint16_t,int){++terminated;return 0;}
inline int ble_sm_inject_io(uint16_t,ble_sm_io* io){assert(io->action==BLE_SM_IOACT_NUMCMP);if(io->numcmp_accept)++accepted;else ++rejected;return 0;}
inline int ble_gap_unpair(const ble_addr_t*){if(storeFail)return 1;bonds=0;return 0;}
struct ble_npl_event {void(*callback)(ble_npl_event*)=nullptr;void* arg=nullptr;bool queued=false;};
inline std::deque<ble_npl_event*> workQueue;
inline void ble_npl_event_init(ble_npl_event* e,void(*cb)(ble_npl_event*),void* arg){e->callback=cb;e->arg=arg;}
inline void* ble_npl_event_get_arg(ble_npl_event* e){return e->arg;}
inline void* nimble_port_get_dflt_eventq(){return nullptr;}
inline void ble_npl_eventq_put(void*,ble_npl_event* e){if(!e->queued){e->queued=true;workQueue.push_back(e);}}
inline void drain(){while(!workQueue.empty()){auto* e=workQueue.front();workQueue.pop_front();e->queued=false;e->callback(e);}}
inline int nimble_port_init(){return 0;}
inline void nimble_port_run(){}inline void nimble_port_freertos_deinit(){}
inline void nimble_port_freertos_init(void(*)(void*)){ble_hs_cfg.sync_cb();}
inline void emit(int type){ble_gap_event event;event.type=type;if(type==BLE_GAP_EVENT_CONNECT)advertising=false;gapCallback(&event,gapContext);}
'''
test = r'''
#include "firmware/vehicle/main/BlePhone.h"
extern "C" void ble_store_config_init(){}
using namespace cluster;using namespace phonelink;
int main(){
 nvsFail=true;board::BlePhone unavailable;assert(!unavailable.initialize());nvsFail=false;
 board::BlePhone phone;assert(phone.initialize());
 assert(std::strcmp(deviceName,"R15 Cluster")==0&&ble_hs_cfg.sm_mitm&&ble_hs_cfg.sm_sc&&ble_hs_cfg.sm_bonding);
 phone.poll(true);drain();assert(phone.snapshot(millis).phone.available());
 emit(BLE_GAP_EVENT_CONNECT);assert(terminated==1);emit(BLE_GAP_EVENT_DISCONNECT);
 assert(phone.post({Action::Pair,0}));phone.poll(true);drain();assert(phone.snapshot(millis).phone.pairWindowOpen);
 emit(BLE_GAP_EVENT_CONNECT);emit(BLE_GAP_EVENT_PASSKEY_ACTION);
 auto s=phone.snapshot(millis);assert(s.phone.pairingRequested&&s.pairing==987&&!s.phone.connected);
 assert(phone.post({Action::Approve,988}));phone.poll(true);drain();assert(!accepted&&phone.snapshot(millis).phone.commandFailed);
 assert(phone.post({Action::Approve,987}));phone.poll(true);drain();assert(accepted==1&&phone.snapshot(millis).phone.pairingApproved);
 descriptor.sec_state={true,false,true,16};emit(BLE_GAP_EVENT_ENC_CHANGE);assert(!phone.snapshot(millis).phone.connected);
 emit(BLE_GAP_EVENT_DISCONNECT);
 assert(phone.post({Action::Pair,0}));phone.poll(true);drain();emit(BLE_GAP_EVENT_CONNECT);emit(BLE_GAP_EVENT_PASSKEY_ACTION);
 assert(phone.post({Action::Approve,987}));phone.poll(true);drain();bonds=1;descriptor.sec_state={true,true,true,16};emit(BLE_GAP_EVENT_ENC_CHANGE);
 s=phone.snapshot(millis);assert(valid(s.phone)&&s.phone.connected&&s.phone.paired&&!s.phone.pairingRequested);
 emit(BLE_GAP_EVENT_DISCONNECT);assert(phone.snapshot(millis).phone.paired&&!phone.snapshot(millis).phone.connected);
 assert(phone.post({Action::Forget,0}));phone.poll(true);drain();assert(!phone.snapshot(millis).phone.paired);
 assert(phone.post({Action::Pair,0}));phone.poll(true);drain();emit(BLE_GAP_EVENT_CONNECT);emit(BLE_GAP_EVENT_PASSKEY_ACTION);
 phone.poll(false);drain();assert(!phone.snapshot(millis).phone.pairingRequested&&!phone.snapshot(millis).phone.pairWindowOpen);
 emit(BLE_GAP_EVENT_DISCONNECT);
 assert(phone.post({Action::Pair,0}));millis+=1100;phone.poll(true);drain();assert(!phone.snapshot(millis).phone.pairWindowOpen&&phone.snapshot(millis).phone.commandFailed);
 assert(phone.post({Action::Pair,0}));phone.poll(true);drain();millis+=60000;phone.poll(true);drain();assert(!phone.snapshot(millis).phone.pairWindowOpen);
 storeFail=true;ble_store_value value{};assert(ble_hs_cfg.store_write_cb(1,&value));phone.poll(true);drain();assert(!phone.snapshot(millis).phone.available());
}
'''
with tempfile.TemporaryDirectory(prefix="r15-ble-check-") as directory:
    temp = Path(directory)
    (temp / "platform.h").write_text(platform)
    names = ["freertos/FreeRTOS.h", "freertos/queue.h", "freertos/portmacro.h", "nimble/nimble_port.h", "nimble/nimble_port_freertos.h", "host/ble_hs.h", "host/ble_gap.h", "host/ble_sm.h", "host/ble_store.h", "services/gap/ble_svc_gap.h", "services/gatt/ble_svc_gatt.h", "esp_timer.h", "esp_random.h", "nvs_flash.h", "sdkconfig.h"]
    for name in names:
        header = temp / name
        header.parent.mkdir(parents=True, exist_ok=True)
        header.write_text('#include "platform.h"\n')
    (temp / "check.cpp").write_text(test)
    subprocess.run(["c++", "-std=c++17", "-I", str(temp), "-I", str(root), "-I", str(root / "firmware/vehicle/main"), str(temp / "check.cpp"), str(root / "firmware/vehicle/main/BlePhone.cpp"), "-o", str(temp / "check")], check=True)
    subprocess.run([str(temp / "check")], check=True)
print("Actual BLE adapter: explicit enrollment, code-bound approval, authentication, bonds, timeout and storage faults pass (fake radio).")
