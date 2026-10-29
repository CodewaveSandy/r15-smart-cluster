#include "BlePhone.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include <cstring>
#include "sdkconfig.h"
static_assert(CONFIG_BT_NIMBLE_SM_LVL==3&&CONFIG_BT_NIMBLE_SM_SC_ONLY==1,"Authenticated SC-only pairing is required");
#ifdef CONFIG_BT_NIMBLE_SM_LEGACY
#error Legacy pairing must remain disabled
#endif
namespace cluster::board {
BlePhone* BlePhone::instance_=nullptr;
namespace {std::uint64_t nowMs(){return std::uint64_t(esp_timer_get_time()/1000);}}
bool BlePhone::initialize() {
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
    ble_store_config_init();write_=ble_hs_cfg.store_write_cb;delete_=ble_hs_cfg.store_delete_cb;
    if(!write_||!delete_)return false;
    ble_hs_cfg.store_write_cb=storeWrite;ble_hs_cfg.store_delete_cb=storeDelete;initialized_=true;nimble_port_freertos_init(task);return true;
}
int BlePhone::storeWrite(int type,const union ble_store_value* value) {
    auto& self=*instance_;const int rc=self.write_(type,value);if(rc)self.storageFault_=true;return rc;
}
int BlePhone::storeDelete(int type,const union ble_store_key* key) {
    auto& self=*instance_;const int rc=self.delete_(type,key);if(rc&&rc!=BLE_HS_ENOENT)self.storageFault_=true;return rc;
}
void BlePhone::task(void*){nimble_port_run();nimble_port_freertos_deinit();}
void BlePhone::reset(int) {
    auto& s=*instance_;s.ready_=false;s.connection_=BLE_HS_CONN_HANDLE_NONE;s.state_={};s.state_.phone.bleBench=true;s.publish();
}
void BlePhone::synced() {
    auto& s=*instance_;if(ble_hs_id_infer_auto(0,&s.addressType_)!=0)return;
    s.ready_=true;s.state_.phone.quality=Quality::Available;s.updateBond();s.advertise();s.publish();
}
void BlePhone::updateBond() {
    int count=0;const int rc=ble_store_util_bonded_peers(&bonded_,&count,1);
    knownBond_=rc==0&&count==1;state_.phone.paired=knownBond_;
    if(rc!=0)state_.phone.quality=Quality::Unavailable;
}
void BlePhone::publish() {
    portENTER_CRITICAL(&mux_);published_=state_;publishedAt_=nowMs();portEXIT_CRITICAL(&mux_);
}
BlePhone::Snapshot BlePhone::snapshot(std::uint64_t now) {
    portENTER_CRITICAL(&mux_);auto result=published_;const auto sampled=publishedAt_;portEXIT_CRITICAL(&mux_);
    if(!sampled||now<sampled||now-sampled>=1000)result.phone.quality=Quality::Unavailable;
    if(!result.phone.available()){result.pairing=result.code=0;result.phone={};result.phone.bleBench=true;}
    return result;
}
bool BlePhone::post(phonelink::Request request) {const Queued queued{request,nowMs()+1000};return initialized_&&xQueueSend(queue_,&queued,0)==pdTRUE;}
void BlePhone::poll(bool displayFresh) {
    if(!initialized_)return;
    portENTER_CRITICAL(&mux_);displayFresh_=displayFresh;displayObservedAt_=nowMs();portEXIT_CRITICAL(&mux_);
    ble_npl_eventq_put(nimble_port_get_dflt_eventq(),&event_);
}
bool BlePhone::displayAvailable(std::uint64_t now) {
    portENTER_CRITICAL(&mux_);const bool fresh=displayFresh_&&now>=displayObservedAt_&&now-displayObservedAt_<1000;portEXIT_CRITICAL(&mux_);return fresh;
}
void BlePhone::work(ble_npl_event* event){static_cast<BlePhone*>(ble_npl_event_get_arg(event))->process();}
void BlePhone::advertise() {
    if(!ready_||connection_!=BLE_HS_CONN_HANDLE_NONE||ble_gap_adv_active())return;
    ble_hs_adv_fields fields{};fields.flags=BLE_HS_ADV_F_DISC_GEN|BLE_HS_ADV_F_BREDR_UNSUP;
    const char* name=ble_svc_gap_device_name();fields.name=reinterpret_cast<const std::uint8_t*>(name);fields.name_len=std::strlen(name);fields.name_is_complete=1;
    if(ble_gap_adv_set_fields(&fields)!=0){state_.phone.quality=Quality::Unavailable;return;}
    ble_gap_adv_params params{};params.conn_mode=BLE_GAP_CONN_MODE_UND;params.disc_mode=BLE_GAP_DISC_MODE_GEN;
    if(ble_gap_adv_start(addressType_,nullptr,BLE_HS_FOREVER,&params,gap,this)!=0)state_.phone.quality=Quality::Unavailable;
}
void BlePhone::reject() {
    if(state_.phone.pairingRequested&&connection_!=BLE_HS_CONN_HANDLE_NONE) {
        ble_sm_io io{};io.action=BLE_SM_IOACT_NUMCMP;io.numcmp_accept=0;ble_sm_inject_io(connection_,&io);
    }
    if(connection_!=BLE_HS_CONN_HANDLE_NONE&&!state_.phone.connected)ble_gap_terminate(connection_,BLE_ERR_REM_USER_CONN_TERM);
    state_.phone.pairingRequested=state_.phone.pairingApproved=false;state_.phone.pairingCode={};state_.pairing=state_.code=0;approved_=false;
}
void BlePhone::process() {
    if(storageFault_){state_.phone.quality=Quality::Unavailable;state_.phone.connected=false;reject();publish();return;}
    const auto now=nowMs();const bool displayFresh=displayAvailable(now);
    if(!displayFresh||now>=windowDeadline_)state_.phone.pairWindowOpen=false;
    if(state_.phone.pairingRequested&&(!displayFresh||now>=pairDeadline_))reject();
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
            if(!knownBond_){rc=-1;break;}
            rc=ble_gap_unpair(&bonded_);updateBond();
            if(knownBond_)rc=-1;else state_.phone.connected=false;
            if(connection_!=BLE_HS_CONN_HANDLE_NONE)ble_gap_terminate(connection_,BLE_ERR_REM_USER_CONN_TERM);
            break;
        default:rc=-1;break;
        }
        if(rc)state_.phone.commandFailed=true;
    }
    if(ready_)advertise();publish();
}
int BlePhone::gap(ble_gap_event* event,void* arg) {
    auto& s=*static_cast<BlePhone*>(arg);
    ble_gap_conn_desc desc{};
    switch(event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if(event->connect.status){s.advertise();break;}
        s.connection_=event->connect.conn_handle;s.approved_=false;
        if(ble_gap_conn_find(s.connection_,&desc)!=0){ble_gap_terminate(s.connection_,BLE_ERR_REM_USER_CONN_TERM);break;}
        // A reconnecting phone may still present an unresolved private address
        // here. The stored identity is checked after authenticated encryption.
        if(!s.knownBond_&&(!s.state_.phone.pairWindowOpen||!s.displayAvailable(nowMs())||nowMs()>=s.windowDeadline_)) {
            ble_gap_terminate(s.connection_,BLE_ERR_REM_USER_CONN_TERM);break;
        }
        if(ble_gap_security_initiate(s.connection_)!=0)ble_gap_terminate(s.connection_,BLE_ERR_REM_USER_CONN_TERM);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        s.connection_=BLE_HS_CONN_HANDLE_NONE;s.state_.phone.connected=false;
        s.state_.phone.pairingRequested=s.state_.phone.pairingApproved=false;s.state_.phone.pairingCode={};s.state_.pairing=s.state_.code=0;s.approved_=false;
        s.updateBond();s.advertise();break;
    case BLE_GAP_EVENT_PASSKEY_ACTION:
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
        s.state_.phone.pairingCode={};s.state_.pairing=s.state_.code=0;break;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        // Never silently discard an existing trusted bond to make pairing work.
        return BLE_GAP_REPEAT_PAIRING_IGNORE;
    case BLE_GAP_EVENT_ADV_COMPLETE:s.advertise();break;
    default:break;
    }
    s.publish();return 0;
}
}
