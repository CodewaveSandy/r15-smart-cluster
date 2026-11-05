#pragma once
#include "shared/protocol/PhoneLink.h"
#include "shared/application/TurnGuidanceBridge.h"
#include "shared/protocol/PhoneMetadata.h"
#include "shared/protocol/PhoneTelemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"
extern "C" {
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_sm.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "host/ble_store.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
void ble_store_config_init(void);
}
namespace cluster::board {
class BlePhone {
public:
    struct Snapshot {PhoneState phone;std::uint32_t pairing=0,code=0;phonemeta::Data metadata;};
    bool initialize(std::uint32_t boot);
    bool post(phonelink::Request request);
    void poll(bool displayFresh,const gearlink::Packet& inputs,std::uint32_t maxSourceGapMs);
    Snapshot snapshot(std::uint64_t now);
    void guidanceDownstream(const guidance::Message&,std::uint64_t now);
    bool takeGuidance(guidance::Message&);
    bool takeGuidanceReset();
    guidance::Bridge::Diagnostics guidanceDiagnostics();
private:
    static BlePhone* instance_;
    static int guidanceAccess(std::uint16_t,std::uint16_t,ble_gatt_access_ctxt*,void*);
    void notifyGuidance(std::uint64_t now);
    guidance::Bridge guidance_;
    std::uint16_t guidanceHandle_=0;
    bool guidanceSubscribed_=false,guidanceNotifyPending_=false;
    std::uint64_t guidanceNotifyAt_=0,guidanceNextNotify_=0;
    static int gap(ble_gap_event*,void*);
    static int streamAccess(std::uint16_t,std::uint16_t,ble_gatt_access_ctxt*,void*);
    bool authenticated(std::uint16_t handle);
    phonetelemetry::Bytes telemetry(std::uint64_t now);
    void notify(std::uint64_t now);
    void resetStream();
    phonetelemetry::Diagnostics diagnostics_;
    gearlink::Packet inputs_;std::uint64_t inputsAt_=0,processAt_=0,lastMetadata_=0;
    std::uint32_t boot_=0,telemetrySequence_=0,sourceGap_=0;
    std::uint16_t telemetryHandle_=0;
    bool subscribed_=false,notifyPending_=false,notifyStalled_=false;
    std::uint64_t nextNotify_=0,notifyAt_=0;
    static int metadataAccess(std::uint16_t,std::uint16_t,ble_gatt_access_ctxt*,void*);
    std::uint64_t metadataAt_=0;
    static int nameAccess(std::uint16_t,std::uint16_t,ble_gatt_access_ctxt*,void*);
    bool securityPending_=false;std::uint64_t securityDue_=0;
    static int storeWrite(int,const union ble_store_value*);static int storeDelete(int,const union ble_store_key*);
    static void synced();static void reset(int);static void task(void*);static void work(ble_npl_event*);
    bool displayAvailable(std::uint64_t now);
    void fault(const char* source,int code,int type=0);
    bool faultLogged_=false,healthLogged_=false,advertiseRequested_=false;
    void beginSecurity();
    void finishForget(std::uint64_t now);
    bool forgetting_=false;std::uint64_t forgetDeadline_=0;
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
