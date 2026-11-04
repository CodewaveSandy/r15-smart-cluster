#pragma once
#include "shared/protocol/MapProtocol.h"
#include "shared/protocol/MapSha256.h"

namespace cluster::maplink {
// One application/LVGL owner. Storage is explicitly supplied from reserved
// PSRAM on the board; no map-sized members/copies on task stacks.
class MapStore {
public:
    struct Diagnostics {
        std::uint32_t commands=0,invalid=0,wrongSession=0,dataBytes=0,duplicates=0,failures=0;
        std::uint64_t receiveMs=0,verifyMs=0,anchorAgeMs=0,sourceAgeUpperMs=0;
        unsigned accuracyCm=UINT32_MAX;LocationState source=LocationState::NoAnchor;
        bool hasMap=false,inside=false;
    };
    Diagnostics diagnostics(std::uint64_t now)const{
        auto d=diagnostics_;d.hasMap=active_;d.inside=fix_&&active_&&activeRegion_.contains(lat_,lon_);
        d.source=sourceState(now);d.accuracyCm=fix_?accuracy_:UINT32_MAX;
        d.anchorAgeMs=anchor_&&now>=anchorAt_?now-anchorAt_:UINT64_MAX;
        d.sourceAgeUpperMs=UINT64_MAX;
        if(anchor_&&fix_&&now>=anchorAt_){
            const auto elapsed=now-anchorAt_;
            const auto behind=phoneBase_>=fixTime_?phoneBase_-fixTime_:0;
            const auto ahead=fixTime_>phoneBase_?fixTime_-phoneBase_:0;
            const auto age=elapsed>=ahead?elapsed-ahead:0;
            d.sourceAgeUpperMs=std::min<std::uint64_t>(60000,age+std::min<std::uint64_t>(60000,behind)+uncertainty_+(elapsed+999)/1000);
        }
        if(state_==State::Receiving&&now>=receiveStarted_)d.receiveMs=now-receiveStarted_;
        if(state_==State::Verifying&&now>=verifyStarted_)d.verifyMs=now-verifyStarted_;
        return d;
    }
    void initialize(std::uint32_t boot,std::uint8_t* binary0,std::uint8_t* binary1,maps::Scene* scene0,maps::Scene* scene1){
        boot_=boot?boot:1;binary_[0]=binary0;binary_[1]=binary1;scenes_[0]=scene0;scenes_[1]=scene1;ready_=binary0&&binary1&&scene0&&scene1;state_=ready_?State::Empty:State::Unsupported;
    }
    void session(std::uint32_t vehicle,std::uint32_t connection,bool link){
        if(vehicle_==vehicle&&connection_==connection&&link_==link)return;
        vehicle_=vehicle;connection_=connection;link_=link;generation_=lastControl_=lastLocation_=maxGeneration_=0;total_=offset_=0;anchor_=fix_=pendingTime_=false;stopped_=false;fixSeen_=false;lastDispatch_=0;location_=LocationState::NoAnchor;lastRequest_.size=0;
        state_=!ready_?State::Unsupported:active_?State::Active:State::Empty;error_=Error::None;dirty_=true;
    }
    Capability capability()const{return maplink::capability(vehicle_,link_?connection_:0,boot_,ready_);}
    bool ready()const{return ready_;}
    maps::Scene* renderStorage(unsigned i)const{return scenes_[i&1];}
    void discard(){active_=false;generation_=maxGeneration_=lastControl_=lastLocation_=0;anchor_=fix_=pendingTime_=false;total_=offset_=0;state_=ready_?State::Empty:State::Unsupported;dirty_=true;}
    const maps::Scene* scene()const{return active_?scenes_[activeSlot_]:nullptr;}
    const Region& region()const{return activeRegion_;}
    bool takeDirty(){const bool v=dirty_;dirty_=false;return v;}
    void command(const Message& m,std::uint64_t now){
        ++diagnostics_.commands;
        if(!validCommand(m)){++diagnostics_.invalid;return;}
        const auto* b=m.bytes.data();const auto type=Type(b[5]);const auto seq=std::uint32_t(get(b,24));
        if(!ready_||!link_||get(b,8)!=vehicle_||get(b,12)!=connection_||get(b,16)!=boot_){++diagnostics_.wrongSession;return;}
        if(type==Type::Location){
            if(!forward(seq,lastLocation_)||stopped_)return;
            const auto* p=b+32;
            if(p[35]!=1){lastLocation_=seq;fix_=false;location_=p[35]==2?LocationState::Stale:(p[34]&4)?LocationState::LowAccuracy:p[35]==3?LocationState::Invalid:LocationState::NoFix;dirty_=true;return;}
            // Never renew freshness with a replayed/backwards sample.
            const auto sample=get(p,0,8),dispatch=get(p,8,8);
            if((fixSeen_&&sample<=fixTime_)||dispatch<lastDispatch_)return;
            lastLocation_=seq;fixTime_=sample;lastDispatch_=dispatch;fixSeen_=true;fix_=true;fixAt_=now;
            lat_=signed32(p,16);lon_=signed32(p,20);accuracy_=unsigned(get(p,24));course_=unsigned(get(p,28,2));courseAccuracy_=unsigned(get(p,30,2));speed_=unsigned(get(p,32,2));dirty_=true;return;
        }
        if(seq==lastControl_){if(m.size==lastRequest_.size&&std::equal(m.bytes.begin(),m.bytes.begin()+m.size,lastRequest_.bytes.begin())){dirty_=true;++diagnostics_.duplicates;}return;}
        if(!forward(seq,lastControl_))return;
        lastControl_=seq;lastRequest_=m;dirty_=true;error_=Error::None;
        const auto gen=std::uint32_t(get(b,20));const auto* body=b+32;
        switch(type){
        case Type::TimeRequest:
            stopped_=false;pendingTime_=true;timeSequence_=seq;phoneT0_=get(body,0,8);displayBase_=now;break;
        case Type::TimeConfirm:{
            const auto t0=get(body,4,8),t1=get(body,12,8);
            if(!pendingTime_||get(body,0)!=timeSequence_||t0!=phoneT0_||now-displayBase_>2000||t1<t0||t1-t0>500){error_=Error::Timing;break;}
            if(anchor_){
                if(t1<phoneBase_||displayBase_<anchorAt_){anchor_=fix_=false;error_=Error::Timing;break;}
                const auto phoneDelta=t1-phoneBase_,displayDelta=displayBase_-anchorAt_;
                const auto gap=phoneDelta>=displayDelta?phoneDelta-displayDelta:displayDelta-phoneDelta;
                uncertainty_=std::max(uncertainty_,std::min<std::uint64_t>(gap,30000)+2);
            }else uncertainty_=2;
            phoneBase_=t1;anchorAt_=displayBase_;anchor_=true;pendingTime_=false;break;}
        case Type::Begin:
            if(state_==State::Receiving||state_==State::Verifying){error_=Error::Busy;break;}
            if(!forward(gen,maxGeneration_)){error_=Error::Session;break;}
            maxGeneration_=generation_=gen;total_=unsigned(get(body,0));offset_=0;std::copy_n(body+4,32,expectedHash_.begin());
            if(active_&&expectedHash_==activeHash_&&total_==activeSize_){activeGeneration_=gen;offset_=total_;state_=State::Active;break;}
            state_=State::Receiving;lastData_=receiveStarted_=now;diagnostics_.receiveMs=diagnostics_.verifyMs=0;break;
        case Type::Data:
            if(state_!=State::Receiving||gen!=generation_){error_=Error::Session;break;}
            if(get(body,0)!=offset_||m.size-40>total_-offset_){fail(Error::Offset);break;}
            std::copy_n(body+4,m.size-40,binary_[1-activeSlot_]+offset_);offset_+=m.size-40;diagnostics_.dataBytes+=m.size-40;lastData_=now;diagnostics_.receiveMs=now-receiveStarted_;break;
        case Type::Commit:
            if(state_!=State::Receiving||gen!=generation_){error_=Error::Session;break;}
            if(offset_!=total_){fail(Error::Offset);break;}
            state_=State::Verifying;verifyStarted_=now;sha_.reset();hashOffset_=0;bodyCrc_=0xffffffffu;parsing_=false;break;
        case Type::Cancel:
            if(gen!=generation_){error_=Error::Session;break;}
            state_=State::Cancelled;total_=offset_=0;break;
        case Type::Stop:
            state_=active_?State::Active:State::Empty;total_=offset_=0;fix_=anchor_=pendingTime_=false;stopped_=true;break;
        default:break;
        }
    }
    void poll(std::uint64_t now){
        if(state_==State::Receiving&&now-lastData_>=15000)fail(Error::Timeout);
        if(state_==State::Verifying){
            diagnostics_.verifyMs=now-verifyStarted_;
            const auto* b=binary_[1-activeSlot_];
            if(hashOffset_<total_){
                const unsigned end=std::min(total_,hashOffset_+512);sha_.update(b+hashOffset_,end-hashOffset_);
                for(unsigned i=std::max(hashOffset_,64u);i<end;++i){bodyCrc_^=b[i];for(unsigned j=0;j<8;++j)bodyCrc_=(bodyCrc_>>1)^((bodyCrc_&1)?0xedb88320u:0);}
                hashOffset_=end;
                if(end==total_&&(sha_.finish()!=expectedHash_||(bodyCrc_^0xffffffffu)!=get(b,44))){fail(Error::Hash);return;}
                return;
            }
            if(!parsing_){if(!reader_.begin(b,total_,*scenes_[1-activeSlot_])){fail(Error::Bounds);return;}parsing_=true;}
            const int result=reader_.step();if(result<0){fail(Error::Bounds);return;}
            if(result==1){activeSlot_=1-activeSlot_;active_=true;activeRegion_=reader_.region;activeHash_=expectedHash_;activeSize_=total_;activeCrc_=std::uint32_t(get(b,44));activeGeneration_=generation_;state_=State::Active;dirty_=true;}
        }
        const auto previous=location_;location_=evaluate(now);if(previous!=location_)dirty_=true;
    }
    LocationState sourceState(std::uint64_t now)const{
        if(stopped_)return LocationState::Stopped;
        if(!link_||!anchor_||now<anchorAt_||now-anchorAt_>=30000)return LocationState::NoAnchor;
        if(!fix_)return location_==LocationState::Stale||location_==LocationState::LowAccuracy||location_==LocationState::Invalid?location_:LocationState::NoFix;
        const auto elapsed=now-anchorAt_;std::int64_t age=0;
        if(phoneBase_>=fixTime_){const auto behind=phoneBase_-fixTime_;if(behind>=2000)return LocationState::Stale;age=std::int64_t(elapsed+behind);}
        else {const auto ahead=fixTime_-phoneBase_;if(ahead>elapsed+500)return LocationState::Stale;age=std::int64_t(elapsed)-std::int64_t(ahead);}
        const auto drift=(elapsed+999)/1000;
        // A sample apparently ahead of dispatch upper bound indicates clock
        // discontinuity/uncertainty, not permission to extend its lifetime.
        if(age< -500||std::uint64_t(std::max<std::int64_t>(0,age))+uncertainty_+drift>=2000||now-fixAt_>=2000)return LocationState::Stale;
        return LocationState::Fresh;
    }
    LocationState evaluate(std::uint64_t now)const{
        const auto source=sourceState(now);if(source!=LocationState::Fresh)return source;
        return !active_||!activeRegion_.contains(lat_,lon_)?LocationState::Outside:LocationState::Fresh;
    }
    maps::Pose pose(std::uint64_t now)const{
        maps::Pose p;if(evaluate(now)!=LocationState::Fresh)return p;
        p.point=activeRegion_.project(lat_,lon_);p.quality=Quality::Available;p.headingDegrees=heading()?course_/100:0;p.directionKnown=heading();return p;
    }
    Status status(std::uint64_t now)const{
        Status b{};prefix(b.data(),'S');b[5]=80;b[6]=std::uint8_t(state_);b[7]=std::uint8_t(error_);
        put(b.data(),8,vehicle_);put(b.data(),12,connection_);put(b.data(),16,boot_);put(b.data(),20,generation_);put(b.data(),24,lastControl_);put(b.data(),28,offset_);put(b.data(),32,total_);put(b.data(),36,activeGeneration_);
        put(b.data(),40,now,8);put(b.data(),48,phoneT0_,8);put(b.data(),56,displayBase_,8);put(b.data(),64,lastLocation_);b[68]=std::uint8_t(evaluate(now));b[69]=ready_&&link_&&state_!=State::Verifying?1:0;put(b.data(),72,activeCrc_);seal(b.data(),80);return b;
    }
private:
    bool heading()const{return course_<36000&&courseAccuracy_<=4500&&speed_!=65535&&speed_>=200;}
    void fail(Error e){state_=State::Failed;error_=e;dirty_=true;++diagnostics_.failures;}
    std::uint8_t* binary_[2]{};maps::Scene* scenes_[2]{};unsigned activeSlot_=0,total_=0,offset_=0,activeSize_=0,hashOffset_=0;
    bool ready_=false,link_=false,active_=false,dirty_=true,anchor_=false,pendingTime_=false,fix_=false,fixSeen_=false,stopped_=false,parsing_=false;
    std::uint32_t boot_=1,vehicle_=0,connection_=0,generation_=0,activeGeneration_=0,maxGeneration_=0,lastControl_=0,lastLocation_=0,timeSequence_=0,bodyCrc_=0,activeCrc_=0;
    std::uint64_t lastData_=0,receiveStarted_=0,verifyStarted_=0,phoneT0_=0,displayBase_=0,phoneBase_=0,anchorAt_=0,fixTime_=0,fixAt_=0,lastDispatch_=0,uncertainty_=2;
    std::int32_t lat_=0,lon_=0;unsigned accuracy_=UINT32_MAX,course_=65535,courseAccuracy_=65535,speed_=65535;
    Diagnostics diagnostics_;
    State state_=State::Unsupported;Error error_=Error::None;LocationState location_=LocationState::NoAnchor;
    Region activeRegion_;PackageReader reader_;Sha256 sha_;Message lastRequest_;std::array<std::uint8_t,32> expectedHash_{},activeHash_{};
};
}
