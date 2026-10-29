#pragma once
#include "shared/protocol/PhoneLink.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
extern "C" {
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_sm.h"
#include "host/ble_store.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
void ble_store_config_init(void);
}
namespace cluster::board {
class BlePhone {
public:
    struct Snapshot {PhoneState phone;std::uint32_t pairing=0,code=0;};
    bool initialize();
    bool post(phonelink::Request request);
    void poll(bool displayFresh);
    Snapshot snapshot(std::uint64_t now);
private:
    static BlePhone* instance_;
    static int gap(ble_gap_event*,void*);
    static int storeWrite(int,const union ble_store_value*);static int storeDelete(int,const union ble_store_key*);
    static void synced();static void reset(int);static void task(void*);static void work(ble_npl_event*);
    bool displayAvailable(std::uint64_t now);
    void advertise();void process();void updateBond();void publish();void reject();
    Snapshot state_,published_;std::uint64_t publishedAt_=0,displayObservedAt_=0,windowDeadline_=0,pairDeadline_=0;
    bool storageFault_=false;ble_store_write_fn* write_=nullptr;ble_store_delete_fn* delete_=nullptr;
    bool initialized_=false,ready_=false,displayFresh_=false,knownBond_=false,approved_=false;
    std::uint16_t connection_=BLE_HS_CONN_HANDLE_NONE;std::uint8_t addressType_=0;
    ble_addr_t bonded_{};portMUX_TYPE mux_=portMUX_INITIALIZER_UNLOCKED;
    ble_npl_event event_{};
    struct Queued {phonelink::Request request;std::uint64_t deadline;};
    StaticQueue_t queueStorage_{};std::uint8_t queueBytes_[4*sizeof(Queued)]{};QueueHandle_t queue_=nullptr;
};
}
