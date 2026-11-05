#pragma once
#include "shared/application/TurnGuidanceStore.h"
#include "shared/application/TurnGuidanceBridge.h"
#include "shared/protocol/UartRecords.h"
#include "embedded/ui/Dashboard.h"
#include <filesystem>
#include <cstdio>
#include <cstring>
namespace cluster::host {
// Explicit developer simulation using the real codec/relay/store/framing. No
// device source, notification schema or physical latency is simulated as proven.
class GuidancePreview {
public:
    GuidancePreview(ui::Dashboard& dashboard,ClusterState state,ui::ViewOptions& view,Lifecycle& life):dashboard_(dashboard),state_(state),view_(view),life_(life){
        store_.initialize(3);store_.session(1,2,true,now_);bridge_.session(1,2,true,now_);announce();
    }
    std::uint64_t now()const{return now_;}
    void step(unsigned ms){now_+=ms;store_.poll(now_);announce();bridge_.session(1,2,true,now_);}
    void open(){
        step(600);++stream_;auto m=base(guidance::Command::Open,4);m.bytes[40]=1;send(m);clock();
    }
    guidance::Message pendingOpen(){
        step(600);++stream_;auto m=base(guidance::Command::Open,4);m.bytes[40]=1;guidance::seal(m.bytes.data(),m.size);announce();bridge_.admit(m,now_);return m;
    }
    void clock(){
        auto request=base(guidance::Command::TimeRequest,8);guidance::put(request.bytes.data(),40,now_+100000,8);const auto t0=now_+100000,db=now_;const auto seq=guidance::get(request.bytes.data(),24);send(request);
        step(20);auto confirm=base(guidance::Command::TimeConfirm,32);guidance::put(confirm.bytes.data(),40,seq);guidance::put(confirm.bytes.data(),48,t0,8);guidance::put(confirm.bytes.data(),56,t0+20,8);guidance::put(confirm.bytes.data(),64,db,8);send(confirm);clockAt_=now_;
    }
    guidance::Message snapshot(guidance::Maneuver maneuver,const char* instruction="Turn left onto Park Street",guidance::Source source=guidance::Source::Guiding,bool known=true){
        auto m=base(guidance::Command::Snapshot,180);auto* b=m.bytes.data()+40;b[0]=std::uint8_t(source);b[1]=std::uint8_t(maneuver);
        unsigned flags=guidance::Instruction|(known?guidance::AgeKnown:0);
        if(maneuver!=guidance::Maneuver::Unknown)flags|=guidance::Direction;
        guidance::put(b,4,guidance::unknown);guidance::put(b,8,guidance::unknown);guidance::put(b,12,guidance::unknown);
        if(source==guidance::Source::Guiding){flags|=guidance::NextDistance|guidance::Road|guidance::RemainingDistance|guidance::RemainingTime|guidance::Summary;guidance::put(b,4,200);guidance::put(b,8,12400);guidance::put(b,12,1320);b[18]=11;std::memcpy(b+132,"Park Street",11);b[19]=9;std::memcpy(b+164,"ETA 21:10",9);}
        if(maneuver==guidance::Maneuver::Roundabout){flags|=guidance::Exit;b[16]=2;}
        guidance::put(b,2,flags,2);b[17]=std::uint8_t(std::strlen(instruction));std::memcpy(b+36,instruction,b[17]);
        guidance::put(b,20,known?now_+100000:guidance::unknownTime,8);guidance::put(b,28,now_+100000,8);guidance::put(m.bytes.data(),28,++revision_);guidance::put(m.bytes.data(),32,known?0:guidance::unknown);return m;
    }
    void send(guidance::Message m){
        guidance::seal(m.bytes.data(),m.size);announce();const auto e=bridge_.admit(m,now_);
        if(e!=guidance::Error::None){std::printf("SIM admission_error=%u sequence=%u\n",unsigned(e),unsigned(guidance::get(m.bytes.data(),24)));return;}
        guidance::Message pending;if(bridge_.take(pending))wire(pending,1);render();last_=m;
    }
    void direct(guidance::Message m,bool partial=false){guidance::seal(m.bytes.data(),m.size);wire(m,1,partial);render();}
    void raw(guidance::Message m){wire(m,1);render();}
    void instrumentState(const ClusterState& state){state_=state;render();}
    void boardReset(){store_=guidance::Store{};store_.initialize(4);store_.session(1,2,true,now_);announce();render();}
    void presence(unsigned flags=7){if(presenceAt_&&now_-presenceAt_<2000)return;presenceAt_=now_;auto m=base(guidance::Command::Presence,4);m.bytes[40]=std::uint8_t(flags);guidance::put(m.bytes.data(),28,store_.view(now_).revision);send(m);}
    void hold(unsigned ms,bool refresh=true){const auto end=now_+ms;while(now_<end){step(unsigned(std::min<std::uint64_t>(2000,end-now_)));if(refresh&&now_-clockAt_>=24000)clock();presence();}}
    void clear(guidance::Reason reason=guidance::Reason::Stop){auto m=base(guidance::Command::Clear,4);m.bytes[40]=std::uint8_t(reason);send(m);}
    void render(){
        store_.poll(now_);dashboard_.guidance(store_.view(now_));dashboard_.update(state_,view_,life_,now_);lv_tick_inc(20);lv_timer_handler();store_.published(now_,dashboard_.guidanceVisible());wire(guidance::message(store_.status(now_)),2);
    }
    void warning(bool value){state_.engineWarning={value,cluster::Quality::Available};render();}
    guidance::Message last()const{return last_;}
    void diagnostics(const char* phase){const auto d=store_.diagnostics();const auto r=bridge_.diagnostics();std::printf("SIM phase=%s accepted=%u rejected=%u duplicate=%u expired=%u relay_accepted=%u relay_rejected=%u store_bytes=%zu bridge_bytes=%zu assembly_bytes=%zu\n",phase,d.accepted,d.rejected,d.duplicates,d.expired,r.accepted,r.rejected,sizeof store_,sizeof bridge_,sizeof rxAssembly_);}
private:
    guidance::Message base(guidance::Command type,unsigned body){return guidance::command(type,1,2,3,stream_,++sequence_,body);}
    void announce(){wire(guidance::message(store_.capability(now_)),2);}
    void wire(const guidance::Message& m,unsigned direction,bool partial=false){
        ++wireSequence_;
        for(unsigned offset=0;offset<m.size;offset+=48){
            const auto f=uartrecords::fragment(m.bytes.data(),m.size,direction,wireSequence_,offset);
            for(unsigned i=0;i<f.size;++i)(direction==1?rx_:returnRx_).push(f.bytes[i],now_,[](const std::uint8_t*,unsigned,std::uint64_t){},[&](const std::uint8_t* raw,unsigned n,std::uint64_t at){
                (direction==1?rxAssembly_:returnAssembly_).push(raw,n,at,direction,[&](const std::uint8_t* b,unsigned size,std::uint64_t stamp){
                    guidance::Message received;received.size=size;std::copy_n(b,size,received.bytes.begin());
                    if(direction==1)store_.consume(received,stamp);else bridge_.downstream(received,stamp);
                });
            });
            if(partial)break;
        }
    }
    ui::Dashboard& dashboard_;ClusterState state_;ui::ViewOptions& view_;Lifecycle& life_;
    guidance::Store store_;guidance::Bridge bridge_;uartrecords::Receiver rx_,returnRx_;uartrecords::Assembler rxAssembly_,returnAssembly_;
    std::uint64_t now_=10000,clockAt_=0,presenceAt_=0;std::uint32_t sequence_=0,revision_=0,stream_=10;unsigned wireSequence_=0;guidance::Message last_;
};
template<class Capture> void renderGuidance(ui::Dashboard& dashboard,const ClusterState& sample,ui::ViewOptions& view,Lifecycle& life,const std::filesystem::path& output,Capture capture){
    using guidance::Maneuver;using guidance::Source;
    std::filesystem::create_directories(output);
    for(bool track:{false,true})for(bool light:{false,true}){
        view={};view.track=track;view.light=light;view.simulated=true;view.time="20:55";view.date="05 Nov 2026";life={};life.key(true,0);life.advance(2000);
        GuidancePreview sim(dashboard,sample,view,life);
        const auto suffix=std::string(track?"track":"street")+(light?"-day":"-night");
        const auto show=[&](const char* name){sim.render();capture(output/(std::string(name)+"-"+suffix+".bmp"));};
        show("inactive");sim.open();show("waiting");
        for(unsigned i=0;i<=8;++i){sim.step(600);sim.send(sim.snapshot(Maneuver(i),i==4?"Arrive at your destination":"Continue on the indicated road",i==4?Source::Arrived:Source::Guiding));show(("maneuver-"+std::to_string(i)).c_str());}
        sim.step(600);sim.send(sim.snapshot(Maneuver::Unknown,"Follow the notification text"));show("text-only");
        sim.step(600);auto zero=sim.snapshot(Maneuver::Right,"Turn right now");guidance::put(zero.bytes.data(),44,0);sim.send(zero);show("zero-metres");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Left,"Turn left at the next junction onto the very long road name shown by Google Maps notification"));show("long-text");
        sim.step(600);auto shortened=sim.snapshot(Maneuver::Right,"Shortened notification text...");guidance::put(shortened.bytes.data(),42,guidance::get(shortened.bytes.data(),42,2)|guidance::Truncated,2);sim.send(shortened);show("truncated-neutral");
        sim.step(600);auto unknown=sim.snapshot(Maneuver::Right,"Continue until the next reported distance");guidance::put(unknown.bytes.data(),42,guidance::get(unknown.bytes.data(),42,2)&~guidance::NextDistance,2);guidance::put(unknown.bytes.data(),44,guidance::unknown);sim.send(unknown);show("unknown-distance");
        sim.step(600);auto maximum=sim.snapshot(Maneuver::Straight,"Continue along the highway");guidance::put(maximum.bytes.data(),44,1000000);sim.send(maximum);show("maximum-distance");
        sim.step(600);auto tooLarge=sim.snapshot(Maneuver::Left,"Invalid distance rejected");guidance::put(tooLarge.bytes.data(),44,1000001);sim.send(tooLarge);show("above-maximum-rejected");
        sim.step(600);auto unicode=sim.snapshot(Maneuver::Left,"Unsupported text rejected");unicode.bytes[76]=0xc3;unicode.bytes[77]=0xa9;sim.send(unicode);show("non-ascii-rejected");
        auto crc=sim.last();crc.bytes[76]^=1;sim.raw(crc);show("inner-crc-rejected");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Unknown,"Recalculating route",Source::Rerouting));show("rerouting");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Unknown,"GPS unavailable",Source::GpsLost));show("gps-lost");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Unknown,"Paused",Source::Paused));show("paused");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Unknown,"Unsupported notification",Source::Unsupported));show("unsupported-language");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Left,"Turn left ahead",Source::Guiding,false));show("unknown-source-age");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Right,"Turn right onto the flyover"));auto old=sim.last();show("fresh");sim.instrumentState({});show("unavailable-instruments");sim.instrumentState(sample);
        sim.send(old);sim.direct(old);show("identical-retry");
        auto conflict=old;conflict.bytes[76]='X';sim.direct(conflict);show("conflicting-retry-rejected");
        sim.step(600);auto future=sim.snapshot(Maneuver::Left,"Future observation");guidance::put(future.bytes.data(),60,sim.now()+1000000,8);guidance::put(future.bytes.data(),68,sim.now()+1000000,8);sim.send(future);show("future-rejected");
        sim.step(600);auto partial=sim.snapshot(Maneuver::Left,"Partial observation hidden");sim.direct(partial,true);show("partial-hidden");
        sim.hold(31000);show("last-update-neutral");sim.hold(90000);show("long-straight-pending");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Left,"Turn left onto a new road"));show("new-content");
        sim.step(2000);sim.presence(15);show("replacement-pending");
        sim.step(600);sim.send(sim.snapshot(Maneuver::Right,"Turn right onto another road"));
        sim.warning(true);show("warning-priority");sim.warning(false);
        view.settings=true;sim.step(280);sim.render();sim.step(280);show("settings-priority");view.settings=false;sim.step(300);sim.render();sim.step(300);show("settings-return");
        auto abandoned=sim.pendingOpen();sim.clear();show("clear-before-open-received");sim.direct(abandoned);show("preempted-open-rejected");
        sim.clear();show("stop-clears");sim.direct(old);show("late-after-stop-rejected");
        sim.open();sim.step(600);sim.send(sim.snapshot(Maneuver::Arrive,"You have arrived",Source::Arrived));show("arrived");sim.hold(5200);show("arrival-expired");
        sim.open();sim.step(600);sim.send(sim.snapshot(Maneuver::Left,"Turn left ahead again"));sim.hold(31000,false);show("clock-expired-neutral");
        sim.step(5001);show("transport-expired");sim.step(3001);show("loss-restores-info");
        sim.open();sim.clear(guidance::Reason::PermissionRevoked);show("permission-revoked");
        life.key(false,sim.now());sim.step(1000);show("shutdown-priority");sim.step(1000);life.advance(sim.now());show("off");life.key(true,sim.now());sim.step(1000);show("boot-priority");
        sim.diagnostics("before-reset");sim.boardReset();show("display-reboot-clears-context");sim.direct(old);show("old-context-rejected");
        sim.diagnostics("after-reset");
    }
    dashboard.guidance({});
}
}
