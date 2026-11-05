#include "BlePhone.h"
#include "esp_timer.h"
#include "host/ble_att.h"
#include "host/ble_hs_mbuf.h"
#include <algorithm>
#include "esp_random.h"
#include "shared/protocol/PairingIdentity.h"
#include "nvs_flash.h"
#include <cstring>
#include "sdkconfig.h"
static_assert(CONFIG_BT_NIMBLE_SM_LVL==3&&CONFIG_BT_NIMBLE_SM_SC_ONLY==1,"Authenticated SC-only pairing is required");
static_assert(CONFIG_BT_NIMBLE_MAX_CONNECTIONS==1&&CONFIG_BT_NIMBLE_MAX_BONDS==1,"Phone service owns exactly one connection and saved bond");
#ifdef CONFIG_BT_NIMBLE_SM_LEGACY
#error Legacy pairing must remain disabled
#endif
namespace cluster::board {
BlePhone* BlePhone::instance_=nullptr;
namespace {std::uint64_t nowMs(){return std::uint64_t(esp_timer_get_time()/1000);}}
bool BlePhone::initialize(std::uint32_t boot) {
    if(!boot)return false;
    boot_=boot;
    instance_=this;state_.phone.bleBench=true;published_=state_;
    // Preserve prior bonds: storage failure does not authorize erasing NVS.
    if(nvs_flash_init()!=ESP_OK||nimble_port_init()!=ESP_OK)return false;
    queue_=xQueueCreateStatic(4,sizeof(Queued),queueBytes_,&queueStorage_);if(!queue_)return false;
    ble_npl_event_init(&event_,work,this);
    ble_hs_cfg.reset_cb=reset;ble_hs_cfg.sync_cb=synced;
    ble_hs_cfg.sm_io_cap=BLE_SM_IO_CAP_DISP_YES_NO;ble_hs_cfg.sm_bonding=1;ble_hs_cfg.sm_mitm=1;ble_hs_cfg.sm_sc=1;
    ble_hs_cfg.sm_our_key_dist=BLE_SM_PAIR_KEY_DIST_ENC|BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist=BLE_SM_PAIR_KEY_DIST_ENC|BLE_SM_PAIR_KEY_DIST_ID;
    ble_svc_gap_init();ble_svc_gatt_init();
    if(ble_svc_gap_device_name_set("R15 Cluster")!=0)return false;
    // Preserve the untrusted enrollment label; connected endpoints below require authentication.
    static const ble_uuid128_t nameService=BLE_UUID128_INIT(0x10,0xc0,0x31,0x25,0x0a,0x9f,0x3b,0xa5,0x67,0x4d,0xd9,0x8f,0x40,0x12,0x3b,0x81);
    static const ble_uuid128_t nameCharacteristic=BLE_UUID128_INIT(0x11,0xc0,0x31,0x25,0x0a,0x9f,0x3b,0xa5,0x67,0x4d,0xd9,0x8f,0x40,0x12,0x3b,0x81);
    static const ble_uuid128_t metadataCharacteristic=BLE_UUID128_INIT(0x12,0xc0,0x31,0x25,0x0a,0x9f,0x3b,0xa5,0x67,0x4d,0xd9,0x8f,0x40,0x12,0x3b,0x81);
    static const ble_uuid128_t capabilityCharacteristic=BLE_UUID128_INIT(0x13,0xc0,0x31,0x25,0x0a,0x9f,0x3b,0xa5,0x67,0x4d,0xd9,0x8f,0x40,0x12,0x3b,0x81);
    static const ble_uuid128_t telemetryCharacteristic=BLE_UUID128_INIT(0x14,0xc0,0x31,0x25,0x0a,0x9f,0x3b,0xa5,0x67,0x4d,0xd9,0x8f,0x40,0x12,0x3b,0x81);
    static const ble_uuid128_t healthCharacteristic=BLE_UUID128_INIT(0x15,0xc0,0x31,0x25,0x0a,0x9f,0x3b,0xa5,0x67,0x4d,0xd9,0x8f,0x40,0x12,0x3b,0x81);
    static ble_gatt_chr_def characteristics[6]{};
    static ble_gatt_svc_def services[2]{};
    characteristics[0].uuid=&nameCharacteristic.u;characteristics[0].access_cb=nameAccess;
    characteristics[0].flags=BLE_GATT_CHR_F_WRITE;
    characteristics[1].uuid=&metadataCharacteristic.u;characteristics[1].access_cb=metadataAccess;
    characteristics[1].flags=BLE_GATT_CHR_F_WRITE|BLE_GATT_CHR_F_WRITE_ENC|BLE_GATT_CHR_F_WRITE_AUTHEN;
    for(unsigned i=2;i<5;++i){characteristics[i].access_cb=streamAccess;characteristics[i].arg=reinterpret_cast<void*>(std::uintptr_t(i));
        characteristics[i].flags=BLE_GATT_CHR_F_READ|BLE_GATT_CHR_F_READ_ENC|BLE_GATT_CHR_F_READ_AUTHEN;}
    characteristics[2].uuid=&capabilityCharacteristic.u;characteristics[3].uuid=&telemetryCharacteristic.u;
    characteristics[3].flags|=BLE_GATT_CHR_F_NOTIFY;characteristics[3].val_handle=&telemetryHandle_;
    characteristics[4].uuid=&healthCharacteristic.u;
    services[0].type=BLE_GATT_SVC_TYPE_PRIMARY;services[0].uuid=&nameService.u;services[0].characteristics=characteristics;
    if(ble_gatts_count_cfg(services)||ble_gatts_add_svcs(services))return false;
    ble_store_config_init();write_=ble_hs_cfg.store_write_cb;delete_=ble_hs_cfg.store_delete_cb;
    if(!write_||!delete_)return false;
    ble_hs_cfg.store_write_cb=storeWrite;ble_hs_cfg.store_delete_cb=storeDelete;initialized_=true;nimble_port_freertos_init(task);return true;
}
int BlePhone::nameAccess(std::uint16_t handle,std::uint16_t,ble_gatt_access_ctxt* context,void*) {
    auto& s=*instance_;
    if(context->op!=BLE_GATT_ACCESS_OP_WRITE_CHR)return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    const auto now=nowMs();
    if(!s.ready_||s.storageFault_||!s.state_.phone.available()||handle!=s.connection_||s.knownBond_||s.forgetting_||!s.displayAvailable(now)||
       !s.state_.phone.pairWindowOpen||now>=s.windowDeadline_||s.state_.phone.pairingApproved)return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    const unsigned size=OS_MBUF_PKTLEN(context->om);std::array<std::uint8_t,32> bytes{};pairidentity::Name label;
    if(!size||size>bytes.size()||os_mbuf_copydata(context->om,0,size,bytes.data())||!pairidentity::name(bytes.data(),size,label))return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    if(s.state_.phone.displayName[0]&&s.state_.phone.displayName!=label)return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    s.state_.phone.displayName=label;
    // Finish the ATT response before beginning security on the host's next turn.
    if(s.securityPending_)s.securityDue_=now+20;
    s.publish();return 0;
}
int BlePhone::metadataAccess(std::uint16_t handle,std::uint16_t,ble_gatt_access_ctxt* context,void*) {
    auto& s=*instance_;const auto now=nowMs();
    if(context->op!=BLE_GATT_ACCESS_OP_WRITE_CHR||!s.authenticated(handle)) {
        phonetelemetry::increment(s.diagnostics_.rejected);return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    }
    phonemeta::Payload bytes{};phonemeta::Data data;
    if(OS_MBUF_PKTLEN(context->om)!=bytes.size()||os_mbuf_copydata(context->om,0,bytes.size(),bytes.data())||!phonemeta::decodePayload(bytes,data)) {
        phonetelemetry::increment(s.diagnostics_.invalid);return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    if(s.metadataAt_&&now-s.metadataAt_<1000) {phonetelemetry::increment(s.diagnostics_.rateLimited);return BLE_ATT_ERR_WRITE_NOT_PERMITTED;}
    if(s.lastMetadata_)s.diagnostics_.maxMetadataGapMs=std::max(s.diagnostics_.maxMetadataGapMs,std::uint32_t(std::min<std::uint64_t>(now-s.lastMetadata_,UINT32_MAX)));
    s.lastMetadata_=now;phonetelemetry::increment(s.diagnostics_.accepted);
    s.state_.metadata=data;s.metadataAt_=now;s.publish();return 0;
}
bool BlePhone::authenticated(std::uint16_t handle) {
    ble_gap_conn_desc desc{};
    return ready_&&!storageFault_&&!forgetting_&&state_.phone.available()&&state_.phone.connected&&handle==connection_&&
        !ble_gap_conn_find(handle,&desc)&&desc.sec_state.encrypted&&desc.sec_state.authenticated&&desc.sec_state.bonded&&desc.sec_state.key_size==16;
}
int BlePhone::streamAccess(std::uint16_t handle,std::uint16_t,ble_gatt_access_ctxt* context,void* arg) {
    auto& s=*instance_;const auto kind=reinterpret_cast<std::uintptr_t>(arg);
    if(context->op!=BLE_GATT_ACCESS_OP_READ_CHR||!s.authenticated(handle))return BLE_ATT_ERR_READ_NOT_PERMITTED;
    if(context->offset)return BLE_ATT_ERR_INVALID_OFFSET;
    const auto now=nowMs();
    if(kind==2){const auto b=phonetelemetry::capabilities(s.boot_);return os_mbuf_append(context->om,b.data(),b.size())?BLE_ATT_ERR_INSUFFICIENT_RES:0;}
    const auto mtu=ble_att_mtu(handle);
    if(kind==3){if(mtu<phonetelemetry::minimumMtu)return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        const auto b=s.telemetry(now);return os_mbuf_append(context->om,b.data(),b.size())?BLE_ATT_ERR_INSUFFICIENT_RES:0;}
    if(kind==4){if(mtu<phonetelemetry::healthMtu)return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        const auto b=phonetelemetry::diagnostics(s.diagnostics_,s.boot_,s.state_.phone.connectionId,std::uint32_t(now));
        return os_mbuf_append(context->om,b.data(),b.size())?BLE_ATT_ERR_INSUFFICIENT_RES:0;}
    return BLE_ATT_ERR_READ_NOT_PERMITTED;
}
phonetelemetry::Bytes BlePhone::telemetry(std::uint64_t now) {
    phonetelemetry::Snapshot sample;sample.boot=boot_;sample.connection=state_.phone.connectionId;
    sample.sequence=++telemetrySequence_;
    portENTER_CRITICAL(&mux_);sample.inputs=inputs_;const auto received=inputsAt_;portEXIT_CRITICAL(&mux_);
    // Acquisition can publish after this host turn's timestamp was captured.
    const auto observed=nowMs();if(observed>now)now=observed;
    sample.uptime=std::uint32_t(now);sample.displayFresh=displayAvailable(now);
    sample.metadataFresh=metadataAt_&&now>=metadataAt_&&now-metadataAt_<90000;
    if(lastMetadata_&&now>=lastMetadata_)sample.metadataAgeSeconds=phonetelemetry::age((now-lastMetadata_)/1000);
    if(received&&now>=received)sample.sourceAge=phonetelemetry::age(now-received);
    return phonetelemetry::encode(sample);
}
void BlePhone::resetStream() {
    subscribed_=notifyPending_=notifyStalled_=false;nextNotify_=notifyAt_=0;lastMetadata_=0;metadataAt_=0;
}
void BlePhone::notify(std::uint64_t now) {
    if(notifyPending_) {
        if(!notifyStalled_&&now>=notifyAt_&&now-notifyAt_>=1000){notifyStalled_=true;phonetelemetry::increment(diagnostics_.notifyErrors);}
        return; // Missing completion cannot grow an application notification backlog.
    }
    if(!subscribed_||now<nextNotify_||!authenticated(connection_)||ble_att_mtu(connection_)<phonetelemetry::minimumMtu)return;
    nextNotify_=now+phonetelemetry::intervalMs;
    const auto bytes=telemetry(now);auto* mbuf=ble_hs_mbuf_from_flat(bytes.data(),bytes.size());
    if(!mbuf){phonetelemetry::increment(diagnostics_.notifyErrors);return;}
    notifyPending_=true;notifyStalled_=false;notifyAt_=now;
    const int rc=ble_gatts_notify_custom(connection_,telemetryHandle_,mbuf); // NimBLE consumes mbuf even on error.
    if(rc){if(notifyPending_)phonetelemetry::increment(diagnostics_.notifyErrors);notifyPending_=false;}
    else phonetelemetry::increment(diagnostics_.notifyQueued);
}
void BlePhone::fault(const char* source,int code,int type) {
    if(!faultLogged_)std::printf("@BLE FAULT source=%s code=%d type=%d\n",source,code,type);
    faultLogged_=true;state_.phone.quality=Quality::Unavailable;
}
int BlePhone::storeWrite(int type,const union ble_store_value* value) {
    auto& self=*instance_;const int rc=self.write_(type,value);if(rc){self.storageFault_=true;self.fault("store-write",rc,type);}return rc;
}
int BlePhone::storeDelete(int type,const union ble_store_key* key) {
    auto& self=*instance_;const int rc=self.delete_(type,key);if(rc&&rc!=BLE_HS_ENOENT){self.storageFault_=true;self.fault("store-delete",rc,type);}return rc;
}
void BlePhone::task(void*){nimble_port_run();nimble_port_freertos_deinit();}
void BlePhone::reset(int reason) {
    auto& s=*instance_;s.resetStream();s.fault("host-reset",reason);s.securityPending_=false;s.forgetting_=false;s.ready_=false;s.advertiseRequested_=false;s.connection_=BLE_HS_CONN_HANDLE_NONE;s.state_={};s.state_.phone.bleBench=true;s.publish();
}
void BlePhone::synced() {
    auto& s=*instance_;if(ble_hs_id_infer_auto(0,&s.addressType_)!=0)return;
    s.ready_=true;s.state_.phone.quality=Quality::Available;s.updateBond();s.advertiseRequested_=true;s.advertise();s.publish();
}
void BlePhone::updateBond() {
    int count=0;const int rc=ble_store_util_bonded_peers(&bonded_,&count,1);
    knownBond_=rc==0&&count==1;state_.phone.paired=knownBond_;
    if(rc!=0)fault("bond-read",rc);
}
void BlePhone::publish() {
    portENTER_CRITICAL(&mux_);published_=state_;publishedAt_=nowMs();
    if(published_.metadata.epoch&&metadataAt_&&publishedAt_>=metadataAt_) {
        const auto epoch=std::uint64_t(published_.metadata.epoch)+(publishedAt_-metadataAt_)/1000;
        published_.metadata.epoch=epoch<=4102444799u?std::uint32_t(epoch):0;
        if(!published_.metadata.epoch)published_.metadata.offset=0;
    }portEXIT_CRITICAL(&mux_);
}
BlePhone::Snapshot BlePhone::snapshot(std::uint64_t now) {
    portENTER_CRITICAL(&mux_);auto result=published_;const auto sampled=publishedAt_;portEXIT_CRITICAL(&mux_);
    // app_main's cycle timestamp can precede a concurrently published host
    // snapshot. Measure age at observation, not against that earlier timestamp.
    const auto observed=nowMs();if(observed>now)now=observed;
    if(!sampled||now<sampled||now-sampled>=1000) {
        if(sampled&&!healthLogged_)std::printf("@BLE FAULT source=host-health age_ms=%llu\n",static_cast<unsigned long long>(now>=sampled?now-sampled:0));
        healthLogged_=true;result.phone.quality=Quality::Unavailable;
    } else healthLogged_=false;
    if(result.metadata.epoch&&sampled&&now>=sampled) {
        const auto epoch=std::uint64_t(result.metadata.epoch)+(now-sampled)/1000;
        result.metadata.epoch=epoch<=4102444799u?std::uint32_t(epoch):0;
        if(!result.metadata.epoch)result.metadata.offset=0;
    }
    if(!result.phone.available()){result.pairing=result.code=0;result.phone={};result.phone.bleBench=true;}
    return result;
}
bool BlePhone::post(phonelink::Request request) {const Queued queued{request,nowMs()+1000};return initialized_&&xQueueSend(queue_,&queued,0)==pdTRUE;}
void BlePhone::poll(bool displayFresh,const gearlink::Packet& inputs,std::uint32_t maxSourceGapMs) {
    if(!initialized_)return;
    portENTER_CRITICAL(&mux_);displayFresh_=displayFresh;displayObservedAt_=nowMs();
    if(inputs.session==boot_){inputs_=inputs;inputsAt_=displayObservedAt_;sourceGap_=maxSourceGapMs;}
    portEXIT_CRITICAL(&mux_);
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(),&event_);
}
bool BlePhone::displayAvailable(std::uint64_t now) {
    portENTER_CRITICAL(&mux_);
    const auto observed=nowMs();if(observed>now)now=observed;
    const bool fresh=displayFresh_&&now>=displayObservedAt_&&now-displayObservedAt_<1000;
    portEXIT_CRITICAL(&mux_);return fresh;
}
void BlePhone::work(ble_npl_event* event){static_cast<BlePhone*>(ble_npl_event_get_arg(event))->process();}
void BlePhone::advertise() {
    // Advertising can stop and the sole connection slot can be reserved before
    // IDF delivers CONNECT. Only explicit lifecycle events request a restart;
    // periodic work must not interpret inactive advertising as an idle radio.
    if(forgetting_||!advertiseRequested_||!ready_||connection_!=BLE_HS_CONN_HANDLE_NONE||ble_gap_adv_active())return;
    ble_hs_adv_fields fields{};fields.flags=BLE_HS_ADV_F_DISC_GEN|BLE_HS_ADV_F_BREDR_UNSUP;
    const char* name=ble_svc_gap_device_name();fields.name=reinterpret_cast<const std::uint8_t*>(name);fields.name_len=std::strlen(name);fields.name_is_complete=1;
    const int fieldsResult=ble_gap_adv_set_fields(&fields);
    if(fieldsResult){fault("advertise-fields",fieldsResult);return;}
    ble_gap_adv_params params{};params.conn_mode=BLE_GAP_CONN_MODE_UND;params.disc_mode=BLE_GAP_DISC_MODE_GEN;
    const int startResult=ble_gap_adv_start(addressType_,nullptr,BLE_HS_FOREVER,&params,gap,this);
    if(startResult)fault("advertise-start",startResult);
    else advertiseRequested_=false;
}
void BlePhone::reject() {
    if(state_.phone.pairingRequested&&connection_!=BLE_HS_CONN_HANDLE_NONE) {
        ble_sm_io io{};io.action=BLE_SM_IOACT_NUMCMP;io.numcmp_accept=0;ble_sm_inject_io(connection_,&io);
    }
    if(connection_!=BLE_HS_CONN_HANDLE_NONE&&!state_.phone.connected)ble_gap_terminate(connection_,BLE_ERR_REM_USER_CONN_TERM);
    state_.phone.pairingRequested=state_.phone.pairingApproved=false;state_.phone.pairingCode={};state_.pairing=state_.code=0;approved_=false;
}
void BlePhone::beginSecurity() {
    const int result=ble_gap_security_initiate(connection_);
    // Android may already have started SMP (createBond / nRF Bond). In the
    // pinned NimBLE, EALREADY leaves that existing procedure intact. Never
    // terminate it just because our optional metadata timer also fired.
    if(result==BLE_HS_EALREADY)std::printf("@BLE SECURITY peer-procedure-active\n");
    if(result&&result!=BLE_HS_EALREADY) {
        std::printf("@BLE SECURITY start-failed code=%d\n",result);
        state_.phone.commandFailed=true;
        ble_gap_terminate(connection_,BLE_ERR_REM_USER_CONN_TERM);
    }
}
void BlePhone::process() {
    if(storageFault_){state_.phone.quality=Quality::Unavailable;state_.phone.connected=false;reject();publish();return;}
    const auto now=nowMs();const bool displayFresh=displayAvailable(now);
    if(processAt_&&now>=processAt_)diagnostics_.maxHostGapMs=std::max(diagnostics_.maxHostGapMs,std::uint32_t(std::min<std::uint64_t>(now-processAt_,UINT32_MAX)));
    processAt_=now;
    portENTER_CRITICAL(&mux_);const auto sourceGap=sourceGap_;portEXIT_CRITICAL(&mux_);
    diagnostics_.maxSourceGapMs=std::max(diagnostics_.maxSourceGapMs,sourceGap);
    if(!displayFresh||now>=windowDeadline_)state_.phone.pairWindowOpen=false;
    if(state_.phone.pairingRequested&&(!displayFresh||now>=pairDeadline_))reject();
    if(securityPending_&&now>=securityDue_) {
        securityPending_=false;
        if(connection_!=BLE_HS_CONN_HANDLE_NONE) {
            if(!displayFresh||!state_.phone.pairWindowOpen)ble_gap_terminate(connection_,BLE_ERR_REM_USER_CONN_TERM);
            else beginSecurity();
        }
    }
    if(metadataAt_&&now-metadataAt_>=90000){state_.metadata={};metadataAt_=0;phonetelemetry::increment(diagnostics_.expired);
        std::printf("@BLE SYNC metadata-expired\n");}
    Queued queued;
    for(unsigned i=0;i<4&&xQueueReceive(queue_,&queued,0)==pdTRUE;++i) {
        const auto request=queued.request;
        if(!ready_||!state_.phone.available()||now>queued.deadline){state_.phone.commandFailed=true;continue;}
        state_.phone.commandFailed=false;int rc=0;
        switch(request.action) {
        case phonelink::Action::Pair:
            if(!displayFresh||knownBond_){rc=-1;break;}
            state_.phone.pairWindowOpen=true;windowDeadline_=now+60000;break;
        case phonelink::Action::Approve:case phonelink::Action::Reject:
            if(!state_.phone.pairingRequested||request.pairing!=state_.pairing){rc=-1;break;}
            if(request.action==phonelink::Action::Reject){reject();break;}
            if(!displayFresh||now>=pairDeadline_){reject();rc=-1;break;}
            {ble_sm_io io{};io.action=BLE_SM_IOACT_NUMCMP;io.numcmp_accept=1;
             rc=ble_sm_inject_io(connection_,&io);if(!rc){approved_=true;state_.phone.pairingApproved=true;}}
            break;
        case phonelink::Action::Disconnect:
            rc=connection_==BLE_HS_CONN_HANDLE_NONE?-1:ble_gap_terminate(connection_,BLE_ERR_REM_USER_CONN_TERM);break;
        case phonelink::Action::Forget:
            if(!knownBond_||forgetting_){rc=-1;break;}
            forgetting_=true;forgetDeadline_=now+3000;
            state_.phone.pairWindowOpen=false;
            // Delete keys only after the active link has actually closed.
            if(connection_!=BLE_HS_CONN_HANDLE_NONE) {
                rc=ble_gap_terminate(connection_,BLE_ERR_REM_USER_CONN_TERM);
                if(rc)forgetting_=false;
            }
            break;
        default:rc=-1;break;
        }
        if(rc)state_.phone.commandFailed=true;
    }
    finishForget(now);
    if(ready_&&!forgetting_)advertise();
    notify(now);publish();
}
void BlePhone::finishForget(std::uint64_t now) {
    if(!forgetting_)return;
    if(now>=forgetDeadline_){forgetting_=false;state_.phone.commandFailed=true;return;}
    if(connection_!=BLE_HS_CONN_HANDLE_NONE)return;
    // NimBLE refuses unpair while advertising is active (resolving-list update).
    const int stopped=ble_gap_adv_active()?ble_gap_adv_stop():0;
    if(stopped){forgetting_=false;state_.phone.commandFailed=true;return;}
    const int result=ble_gap_unpair(&bonded_);updateBond();
    forgetting_=false;state_.phone.commandFailed=result!=0||knownBond_;
    state_.phone.connected=false;advertiseRequested_=true;
}
int BlePhone::gap(ble_gap_event* event,void* arg) {
    auto& s=*static_cast<BlePhone*>(arg);
    ble_gap_conn_desc desc{};
    switch(event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if(event->connect.status){s.advertiseRequested_=true;s.advertise();break;}
        s.resetStream();phonetelemetry::increment(s.diagnostics_.connections);
        s.state_.metadata={};s.state_.phone.connectionId=esp_random();if(!s.state_.phone.connectionId)s.state_.phone.connectionId=1;
        s.connection_=event->connect.conn_handle;s.advertiseRequested_=false;s.approved_=false;s.state_.phone.displayName={};
        if(ble_gap_conn_find(s.connection_,&desc)!=0){ble_gap_terminate(s.connection_,BLE_ERR_REM_USER_CONN_TERM);break;}
        // A reconnecting phone may still present an unresolved private address
        // here. The stored identity is checked after authenticated encryption.
        if(s.forgetting_||(!s.knownBond_&&(!s.state_.phone.pairWindowOpen||!s.displayAvailable(nowMs())||nowMs()>=s.windowDeadline_))) {
            ble_gap_terminate(s.connection_,BLE_ERR_REM_USER_CONN_TERM);break;
        }
        if(s.knownBond_) {
            s.beginSecurity();
        } else {s.securityPending_=true;s.securityDue_=nowMs()+3000;}
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        std::printf("@BLE LINK disconnected reason=%d\n",int(event->disconnect.reason));
        s.resetStream();phonetelemetry::increment(s.diagnostics_.disconnects);
        s.state_.metadata={};s.state_.phone.connectionId=0;
        s.securityPending_=false;s.connection_=BLE_HS_CONN_HANDLE_NONE;s.state_.phone.displayName={};s.state_.phone.connected=false;
        s.state_.phone.pairingRequested=s.state_.phone.pairingApproved=false;s.state_.phone.pairingCode={};s.state_.pairing=s.state_.code=0;s.approved_=false;
        s.updateBond();if(!s.forgetting_){s.advertiseRequested_=true;s.advertise();}break;
    case BLE_GAP_EVENT_PASSKEY_ACTION:
        s.securityPending_=false;
        if(event->passkey.params.action!=BLE_SM_IOACT_NUMCMP||!s.state_.phone.pairWindowOpen||!s.displayAvailable(nowMs())||nowMs()>=s.windowDeadline_||s.knownBond_) {
            ble_gap_terminate(s.connection_,BLE_ERR_REM_USER_CONN_TERM);break;
        }
        s.state_.pairing=esp_random();if(!s.state_.pairing)s.state_.pairing=1;
        s.state_.code=event->passkey.params.numcmp;s.state_.phone.pairingRequested=true;s.state_.phone.pairingApproved=false;
        std::snprintf(s.state_.phone.pairingCode.data(),7,"%06u",unsigned(s.state_.code));s.pairDeadline_=nowMs()+30000;
        break;
    case BLE_GAP_EVENT_ENC_CHANGE:
        if(s.storageFault_||event->enc_change.status||ble_gap_conn_find(event->enc_change.conn_handle,&desc)!=0||
           !desc.sec_state.encrypted||!desc.sec_state.authenticated||!desc.sec_state.bonded||desc.sec_state.key_size!=16||(!s.knownBond_&&!s.approved_)) {
            s.state_.phone.commandFailed=true;s.state_.phone.connected=false;s.reject();break;
        }
        s.updateBond();
        if(!s.knownBond_||(desc.peer_id_addr.type&1)!=(s.bonded_.type&1)||std::memcmp(desc.peer_id_addr.val,s.bonded_.val,6)!=0) {
            s.state_.phone.commandFailed=true;ble_gap_terminate(s.connection_,BLE_ERR_REM_USER_CONN_TERM);break;
        }
        s.state_.phone.commandFailed=false;s.state_.phone.connected=true;s.state_.phone.pairWindowOpen=false;s.state_.phone.pairingRequested=s.state_.phone.pairingApproved=false;
        s.state_.phone.pairingCode={};s.state_.pairing=s.state_.code=0;
        ble_svc_gatt_changed(1,0xffff);break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if(event->subscribe.conn_handle==s.connection_&&event->subscribe.attr_handle==s.telemetryHandle_) {
            s.subscribed_=event->subscribe.cur_notify; // CCCD rewrites cannot bypass the 200 ms cadence.
        }
        break;
    case BLE_GAP_EVENT_NOTIFY_TX:
        if(event->notify_tx.conn_handle==s.connection_&&event->notify_tx.attr_handle==s.telemetryHandle_&&!event->notify_tx.indication) {
            s.notifyPending_=s.notifyStalled_=false;
            if(event->notify_tx.status)phonetelemetry::increment(s.diagnostics_.notifyErrors);
            else phonetelemetry::increment(s.diagnostics_.notifyCompleted);
        }
        break;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        // Never silently discard an existing trusted bond to make pairing work.
        return BLE_GAP_REPEAT_PAIRING_IGNORE;
    case BLE_GAP_EVENT_ADV_COMPLETE:s.advertiseRequested_=true;s.advertise();break;
    default:break;
    }
    s.publish();return 0;
}
}
