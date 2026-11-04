#pragma once
#include "shared/protocol/NavigationProtocol.h"
#include "shared/protocol/MapSha256.h"
#include "MapCamera.h"
#include <cstring>
#include <cstdio>

namespace cluster::navlink {
// Sole display application owner; publication and drawing are serialized. Raw
// storage and both immutable render snapshots are preflighted in PSRAM.
class NavigationStore {
public:
    struct Slot {Object object;Digest hash{};std::uint8_t* bytes=nullptr;unsigned size=0;std::uint32_t id=0;std::uint64_t used=0;bool valid=false;};
    struct Metrics {std::uint32_t commands=0,rejected=0,duplicates=0,bytes=0,verified=0,evicted=0,hashFailures=0;};
    void initialize(std::uint32_t boot,std::uint8_t* raw,maps::Scene* a,maps::Scene* b){
        boot_=boot?boot:1;ready_=raw&&a&&b;scenes_[0]=a;scenes_[1]=b;
        if(raw){for(unsigned i=0;i<cacheSlots;++i)slots_[i].bytes=raw+i*maxObject;staging_=raw+cacheSlots*maxObject;}
    }
    void session(std::uint32_t vehicle,std::uint32_t connection,bool connected){
        if(vehicle_==vehicle&&connection_==connection&&link_==connected)return;
        vehicle_=vehicle;connection_=connection;link_=connected;maxNav_=0;close();
    }
    void close(){
        active_=false;nav_=last_=locSequence_=progressSequence_=0;stage_=Stage::Closed;error_=Error::None;
        anchor_=pendingTime_=fix_=seenFix_=knownFix_=false;route_=0;revision_=0;overview_=routeWindow_=pendingOverview_=pendingWindow_=-1;pendingRoute_=0;pendingRevision_=0;
        sample_=dispatch_=progressSample_=phoneBase_=anchorAt_=0;uncertainty_=2;progressGood_=false;reportedFix_=Fix::None;
        incoming_=offset_=total_=0;building_=renderDirty_=routeNeedDirty_=routeNeedOutstanding_=false;dirty_=true;historyCount_=historyNext_=0;interestCount_=haveMask_=0;coverage_=Coverage::Missing;requestedMask_=selectedMask_=0;
        for(auto& s:slots_)s.id=0;
    }
    bool ready()const{return ready_;}bool active()const{return active_;}
    const maps::Scene* scene()const{return published_?scenes_[renderSlot_]:nullptr;}
    maps::Scene* renderStorage(unsigned i)const{return scenes_[i&1];}
    Capability capability()const{return navlink::capability(vehicle_,link_?connection_:0,boot_,ready_,ready_?unsigned((cacheSlots+1)*maxObject+2*sizeof(maps::Scene)+2*maplink::maxPackage):0);}
    const Metrics& metrics()const{return metrics_;}
    std::uint64_t route()const{return route_;}std::uint32_t revision()const{return revision_;}
    Stage stage()const{return stage_;}Error error()const{return error_;}
    unsigned accepted()const{return offset_;}unsigned total()const{return total_;}
    bool following()const{return following_;}
    unsigned attribution()const{return attribution_;}
    Fix fixState(std::uint64_t now)const{
        if(!active_)return stage_==Stage::Stopped?Fix::Stopped:Fix::None;
        if(!link_||!anchor_||now<anchorAt_||now-anchorAt_>=30000)return Fix::NoAnchor;
        if(!fix_)return reportedFix_;
        if(!timely(sample_,now)||now-fixAt_>=2000)return Fix::Stale;
        return permission_==2||accuracy_>5000?Fix::Approximate:Fix::Precise;
    }
    Coverage coverage()const{return coverage_;}
    unsigned accuracyCm()const{return fix_?accuracy_:UINT32_MAX;}
    maps::Pose pose(std::uint64_t now)const{
        maps::Pose p;if(!published_||!fix_)return p;const auto f=fixState(now);
        const bool stale=knownFix_&&f==Fix::Stale&&active_&&anchor_&&now>=fixAt_&&now-fixAt_<10000&&sourceAge(now)<10000;
        if(f!=Fix::Precise&&f!=Fix::Approximate&&!stale)return p;
        p.point=project(maps::mercator(lat_,lon_),*scene());p.quality=stale?Quality::Stale:Quality::Available;
        p.marker=stale?maps::Marker::LastKnown:f==Fix::Approximate?maps::Marker::Approximate:maps::Marker::Normal;
        p.accuracyMetres=unsigned(std::min<std::uint64_t>(1000000,(std::uint64_t(accuracy_)+99)/100));
        p.directionKnown=f==Fix::Precise&&course_<36000&&courseAccuracy_<=4500&&speed_!=65535&&speed_>=200;
        p.headingDegrees=p.directionKnown?course_/100:0;return p;
    }
    bool guidance(std::uint64_t now)const{
        if(!active_||!route_||(pendingRoute_&&(pendingRoute_!=route_||pendingRevision_!=revision_))||routeWindow_<0||!progressGood_||fixState(now)!=Fix::Precise||!timely(progressSample_,now)||now-progressAt_>=2000)return false;
        const auto& w=slots_[routeWindow_].object;if(remaining_>w.distance)return false;const auto along=w.distance-remaining_;
        return along>=w.start&&along<=w.end;
    }
    void guidanceText(char* out,unsigned size,std::uint64_t now)const{
        if(!route_){std::snprintf(out,size,"%s",published_&&scene()->overviewCount?"Cached route preview · not active":"No shared route");return;}
        if(!guidance(now)){std::snprintf(out,size,"Route cached · guidance unavailable");return;}
        const char* names[]={"Continue","Turn left","Turn right","Straight","Arrive","Keep left","Keep right","U-turn"};
        std::snprintf(out,size,"%s · %u m\nEst. %.1f km · %u min",names[maneuver_],unsigned(maneuverDistance_),remaining_/1000.0,unsigned((remainingSeconds_+59)/60));
    }
    void command(const Message& m,std::uint64_t now){
        ++metrics_.commands;if(!navlink::command(m)){++metrics_.rejected;return;}
        const auto* b=m.bytes.data();const auto* p=body(m);const auto op=Op(b[5]);const auto seq=std::uint32_t(get(b,24));
        if(!ready_||!link_||get(b,8)!=vehicle_||get(b,12)!=connection_||get(b,16)!=boot_){++metrics_.rejected;return;}
        if(op==Op::Open){
            if(active_&&get(b,20)==nav_){duplicate(m);return;}
            if(!maplink::forward(std::uint32_t(get(b,20)),maxNav_)){++metrics_.rejected;return;}
            Digest source{};std::copy_n(p,32,source.begin());
            if(source!=source_||attribution_!=get(p,34,2)){for(auto& s:slots_)s.valid=false;source_=source;}
            close();attribution_=unsigned(get(p,34,2));published_=false;maxNav_=nav_=std::uint32_t(get(b,20));active_=true;stage_=Stage::Idle;window_=p[36];remember(m);dirty_=true;renderDirty_=true;return;
        }
        if(get(b,20)!=nav_){++metrics_.rejected;return;}
        if(!active_&&op!=Op::Stop&&op!=Op::End){duplicate(m);return;}
        if(op==Op::Location){location(m,now);return;}
        if(op==Op::Progress){progress(m,now);return;}
        if(seq==last_||!maplink::forward(seq,last_)){duplicate(m);return;}
        const auto next=last_==UINT32_MAX?1:last_+1;
        if(seq!=next&&op!=Op::Stop&&op!=Op::End&&op!=Op::Cancel){error_=Error::Sequence;dirty_=true;return;}
        remember(m);error_=Error::None;dirty_=true;const unsigned object=unsigned(get(b,28));
        switch(op){
        case Op::TimeRequest:timeSequence_=seq;t0_=get(p,0,8);db_=now;pendingTime_=true;break;
        case Op::TimeConfirm:{
            const auto t0=get(p,4,8),t1=get(p,12,8);
            if(!pendingTime_||get(p,0)!=timeSequence_||t0!=t0_||now<db_||now-db_>2000){error_=Error::Timing;break;}
            if(anchor_){
                if(t1<phoneBase_||db_<anchorAt_){anchor_=fix_=false;error_=Error::Timing;break;}
                const auto a=t1-phoneBase_,d=db_-anchorAt_;uncertainty_=std::max(uncertainty_,std::min<std::uint64_t>(30000,a>d?a-d:d-a)+2);
            }else uncertainty_=2;
            phoneBase_=t1;anchorAt_=db_;anchor_=true;pendingTime_=false;break;
        }
        case Op::Begin:{
            Digest hash{};std::copy_n(p+4,32,hash.begin());const unsigned size=unsigned(get(p,0));
            if(stage_==Stage::Receiving||stage_==Stage::Verifying){
                if(stage_==Stage::Receiving&&incoming_==object&&expected_==hash&&total_==size&&expectedKind_==Kind(p[36]))break;
                error_=Error::Busy;break;
            }
            const int hit=find(hash,size);
            if(hit>=0){if(slots_[hit].object.kind!=Kind(p[36])||(id(object)>=0&&id(object)!=hit)){error_=Error::Conflict;break;}if(!routeCandidate(slots_[hit].object))break;bindCandidate(hit);slots_[hit].id=object;slots_[hit].used=now;incoming_=object;offset_=total_=size;stage_=Stage::Have;break;}
            if(id(object)>=0){error_=Error::Conflict;break;}
            incoming_=object;total_=size;offset_=0;expected_=hash;expectedKind_=Kind(p[36]);lastData_=now;stage_=Stage::Receiving;break;
        }
        case Op::Data:{
            const unsigned at=unsigned(get(b,32)),n=bodySize(m);
            if(stage_!=Stage::Receiving||incoming_!=object){error_=Error::Session;break;}
            if(at<offset_&&n<=offset_-at&&std::equal(p,p+n,staging_+at))break;
            if(at!=offset_||n>total_-offset_){error_=Error::Offset;break;}
            std::copy_n(p,n,staging_+offset_);offset_+=n;metrics_.bytes+=n;lastData_=now;break;
        }
        case Op::Commit:
            if(stage_==Stage::Have&&incoming_==object)break;
            if(stage_!=Stage::Receiving||incoming_!=object||offset_!=total_){error_=Error::Offset;break;}
            stage_=Stage::Verifying;sha_.reset();hashOffset_=0;verifyIndex_=0;verifyOffset_=64;crc_=0xffffffffu;headerChecked_=false;break;
        case Op::Cancel:{
            const int cached=id(object);
            if(object==incoming_){stage_=Stage::Idle;incoming_=offset_=total_=0;}
            else if(cached<0||(cached!=pendingOverview_&&cached!=pendingWindow_)){error_=Error::Session;break;}
            if(cached>=0&&cached==pendingOverview_)pendingOverview_=-1;
            if(cached>=0&&cached==pendingWindow_)pendingWindow_=-1;
            if(pendingOverview_<0&&pendingWindow_<0){pendingRoute_=0;pendingRevision_=0;}break;
        }
        case Op::Have:{
            if(stage_==Stage::Receiving||stage_==Stage::Verifying){error_=Error::Busy;break;}
            Digest hash{};std::copy_n(p,32,hash.begin());const int hit=find(hash,unsigned(get(p,32)));
            if(hit>=0){if(id(object)>=0&&id(object)!=hit){error_=Error::Conflict;break;}if(!routeCandidate(slots_[hit].object))break;bindCandidate(hit);slots_[hit].id=object;slots_[hit].used=now;incoming_=object;offset_=total_=slots_[hit].size;stage_=Stage::Have;}
            else {stage_=Stage::Idle;incoming_=object;offset_=total_=0;}break;
        }
        case Op::Activate:activate(p,now);break;
        case Op::Stop:case Op::End:
            active_=false;anchor_=pendingTime_=fix_=false;progressGood_=false;incoming_=offset_=total_=0;stage_=Stage::Stopped;
            pendingRoute_=0;pendingRevision_=0;pendingOverview_=pendingWindow_=-1;routeNeedOutstanding_=routeNeedDirty_=false;
            if(op==Op::End){route_=0;revision_=0;overview_=routeWindow_=-1;renderDirty_=true;}break;
        default:break;
        }
    }
    void poll(std::uint64_t now){
        if(fix_&&future(sample_,now)){fix_=knownFix_=seenFix_=false;sample_=dispatch_=0;reportedFix_=Fix::Invalid;dirty_=true;}
        if(active_&&interestCount_&&haveMask_!=((1u<<interestCount_)-1)&&now-lastInterestReport_>=1000){interestDirty_=true;lastInterestReport_=now;}
        if(active_&&routeNeedOutstanding_&&now-lastRouteNeedReport_>=1000){routeNeedDirty_=true;lastRouteNeedReport_=now;}
        if(stage_==Stage::Receiving&&now-lastData_>=15000)fail(Error::Timeout);
        if(renderDirty_)building_=false;
        if(stage_==Stage::Verifying)verify(now);
        if(renderDirty_)startRender();
        if(building_)renderStep();
        const auto f=fixState(now);if(f!=lastFix_){lastFix_=f;dirty_=true;}
        if(f==Fix::Precise||f==Fix::Approximate)knownFix_=true;
    }
    // The display camera alone generates interests. No phone camera or route
    // origin mutation; at most four keys, coalesced to the latest generation.
    void camera(const maps::MapCamera& camera,std::uint64_t now){
        const bool wasFollowing=following_;following_=camera.following();if(!published_)return;
        const auto c=camera.view();const auto base=maps::mercator(scene()->originLatE7,scene()->originLonE7);
        cameraWorld_={maps::wrapEast(base.east+c.east),std::clamp(base.north+c.north,-maps::worldMetres/2,maps::worldMetres/2)};
        scale_=c.pixelsPerMetre;
        const unsigned z=unsigned(std::clamp(int(std::floor(std::log2(maps::worldMetres*scale_/512))),0,18));
        const unsigned cells=1u<<z;const double size=maps::worldMetres/cells;
        const int cx=int(std::floor((cameraWorld_.east+maps::worldMetres/2)/size));
        const int cy=std::clamp(int(std::floor((maps::worldMetres/2-cameraWorld_.north)/size)),0,int(cells)-1);
        struct Candidate {std::uint32_t x=0,y=0;double distance=0;bool visible=false;};
        std::array<Candidate,25> candidates{};unsigned candidatesCount=0,visibleCount=0,visibleHave=0;
        const int left=int(std::floor((cameraWorld_.east+maps::worldMetres/2-400/scale_)/size));
        const int right=int(std::floor((cameraWorld_.east+maps::worldMetres/2+400/scale_)/size));
        const int top=std::max(0,int(std::floor((maps::worldMetres/2-cameraWorld_.north-150/scale_)/size)));
        const int bottom=std::min(int(cells)-1,int(std::floor((maps::worldMetres/2-cameraWorld_.north+150/scale_)/size)));
        for(int y=std::max(0,top-1);y<=std::min(int(cells)-1,bottom+1);++y){
            for(int x=left-1;x<=std::min(right+1,left+int(cells)-1)&&candidatesCount<candidates.size();++x){
                const auto tx=std::uint32_t((x%int(cells)+int(cells))%int(cells));bool duplicate=false;
                for(unsigned i=0;i<candidatesCount;++i)if(candidates[i].x==tx&&candidates[i].y==unsigned(y))duplicate=true;
                if(duplicate)continue;
                const auto center=maps::tilePoint(z,tx,unsigned(y),2048,2048);
                const double dx=maps::wrapEast(center.east-cameraWorld_.east),dy=center.north-cameraWorld_.north;
                const bool visible=std::abs(dx)<=size/2+400/scale_&&std::abs(dy)<=size/2+150/scale_;
                candidates[candidatesCount++]={tx,unsigned(y),dx*dx+dy*dy,visible};
                if(visible){++visibleCount;if(completeCell(z,tx,unsigned(y)))++visibleHave;}
            }
        }
        std::sort(candidates.begin(),candidates.begin()+candidatesCount,[](const auto& a,const auto& b){return a.visible!=b.visible?a.visible:a.distance<b.distance;});
        std::array<std::uint32_t,4> xs{},ys{};unsigned count=0,mask=0;
        // Missing visible cells first; once complete, only the two nearest halo
        // cells. A cached halo does not trigger an endless ring-download loop.
        for(unsigned i=0;i<candidatesCount&&count<4;++i){const auto& c=candidates[i];
            if(c.visible&&!completeCell(z,c.x,c.y)){xs[count]=c.x;ys[count++]=c.y;}
        }
        if(visibleHave==visibleCount){unsigned halo=0;
            for(unsigned i=0;i<candidatesCount&&count<4&&halo<2;++i){const auto& c=candidates[i];if(c.visible)continue;
                xs[count]=c.x;ys[count]=c.y;if(completeCell(z,c.x,c.y))mask|=1u<<count;++count;++halo;
            }
        }
        if(!count){xs[0]=std::uint32_t((cx+int(cells))%int(cells));ys[0]=unsigned(cy);count=1;if(completeCell(z,xs[0],ys[0]))mask=1;}

        if(xs!=interestX_||ys!=interestY_||z!=interestZ_||count!=interestCount_||mask!=haveMask_||following_!=wasFollowing){
            interestX_=xs;interestY_=ys;interestZ_=z;interestCount_=count;haveMask_=mask;if(!++interestGeneration_)++interestGeneration_;interestDirty_=true;
        }
        selectedMask_=selectCells();
        coverage_=visibleCount&&visibleHave==visibleCount&&!renderClipped_?Coverage::Ready:selectedMask_?Coverage::Partial:Coverage::Missing;
        if(selectedMask_!=requestedMask_){requestedMask_=selectedMask_;renderDirty_=true;}
        for(unsigned i=0;i<cacheSlots;++i)if(selectedMask_&(1u<<i))slots_[i].used=now;
    }
    Message status(std::uint64_t now)const{
        auto m=message(64,true,vehicle_,connection_,boot_,nav_,last_,incoming_,offset_,40);auto* p=m.bytes.data()+40;
        p[0]=std::uint8_t(stage_);p[1]=std::uint8_t(error_);p[2]=std::uint8_t(window_);p[3]=active_&&stage_!=Stage::Verifying?std::uint8_t(window_):0;
        put(p,4,total_);put(p,8,revision_);p[12]=std::uint8_t(fixState(now));p[13]=std::uint8_t(coverage_);p[14]=following_?0:1;p[15]=renderClipped_?1:0;put(p,16,now,8);put(p,24,t0_,8);put(p,32,db_,8);seal(m.bytes.data(),m.size);return m;
    }
    Message interest()const{
        auto m=message(65,true,vehicle_,connection_,boot_,nav_,interestGeneration_,0,0,44);auto* p=m.bytes.data()+40;
        put(p,0,interestGeneration_);put(p,4,get(source_.data(),0));p[8]=std::uint8_t(interestZ_);p[9]=std::uint8_t(interestCount_);p[10]=std::uint8_t(haveMask_);p[11]=following_?0:1;
        for(unsigned i=0;i<interestCount_;++i){put(p,12+i*8,interestX_[i]);put(p,16+i*8,interestY_[i]);}seal(m.bytes.data(),m.size);return m;
    }
    bool takeDirty(){const bool b=dirty_;dirty_=false;return b;}
    bool takeInterest(){const bool b=interestDirty_;interestDirty_=false;return active_&&b;}
    bool takeRouteNeed(){const bool b=routeNeedDirty_;routeNeedDirty_=false;return active_&&b;}
    Message routeNeed()const{
        auto m=message(66,true,vehicle_,connection_,boot_,nav_,routeNeedGeneration_,0,0,24);auto* p=m.bytes.data()+40;
        put(p,0,route_,8);put(p,8,revision_);put(p,12,requestedAlong_);if(routeWindow_>=0){put(p,16,slots_[routeWindow_].object.start);put(p,20,slots_[routeWindow_].object.end);}seal(m.bytes.data(),m.size);return m;
    }
private:
    static maps::Point project(maps::WorldPoint p,const maps::Scene& s){const auto b=maps::mercator(s.originLatE7,s.originLonE7);return {std::int32_t(std::lround(maps::wrapEast(p.east-b.east))),std::int32_t(std::lround(p.north-b.north))};}
    std::uint64_t sourceAge(std::uint64_t now)const{
        if(!anchor_||now<anchorAt_)return UINT64_MAX;
        const auto elapsed=now-anchorAt_;
        if(phoneBase_>=sample_)return elapsed+std::min<std::uint64_t>(60000,phoneBase_-sample_);
        const auto ahead=sample_-phoneBase_;return ahead>elapsed?0:elapsed-ahead;
    }
    bool timely(std::uint64_t sample,std::uint64_t now)const{
        if(!anchor_||now<anchorAt_||now-anchorAt_>=30000)return false;
        const auto elapsed=now-anchorAt_;std::uint64_t age=0;
        if(phoneBase_>=sample){const auto delta=phoneBase_-sample;if(delta>=2000)return false;age=elapsed+delta;}
        else {const auto delta=sample-phoneBase_;if(delta>elapsed+500)return false;age=delta>elapsed?0:elapsed-delta;}
        return age+uncertainty_+(elapsed+999)/1000<2000;
    }
    bool future(std::uint64_t sample,std::uint64_t now)const{return anchor_&&now>=anchorAt_&&sample>phoneBase_&&sample-phoneBase_>now-anchorAt_+500;}
    void location(const Message& m,std::uint64_t now){
        const auto seq=std::uint32_t(get(m.bytes.data(),24));if(!maplink::forward(seq,locSequence_))return;
        const auto* p=body(m);const auto sample=get(p,0,8),dispatch=get(p,8,8);
        if(p[35]!=1){locSequence_=seq;fix_=false;reportedFix_=p[35]==2?Fix::Stale:p[35]==3?Fix::Invalid:Fix::None;dirty_=true;return;}
        if((seenFix_&&sample<=sample_)||dispatch<dispatch_)return;
        if(future(sample,now)){locSequence_=seq;fix_=knownFix_=false;reportedFix_=Fix::Invalid;dirty_=true;return;}
        seenFix_=fix_=true;knownFix_=false;locSequence_=seq;sample_=sample;dispatch_=dispatch;fixAt_=now;lat_=signed32(p,16);lon_=signed32(p,20);accuracy_=unsigned(get(p,24));permission_=p[34];
        course_=unsigned(get(p,28,2));courseAccuracy_=unsigned(get(p,30,2));speed_=unsigned(get(p,32,2));dirty_=true;
        if(!published_)renderDirty_=true;
    }
    void progress(const Message& m,std::uint64_t now){
        const auto seq=std::uint32_t(get(m.bytes.data(),24));const auto* p=body(m);
        if(!maplink::forward(seq,progressSequence_)||get(p,0,8)!=route_||get(p,8)!=revision_||get(p,12,8)<=progressSample_)return;
        progressSequence_=seq;progressSample_=get(p,12,8);progressAt_=now;progressGood_=p[33]==1&&timely(progressSample_,now);
        remaining_=std::uint32_t(get(p,20));remainingSeconds_=std::uint32_t(get(p,24));maneuverDistance_=std::uint32_t(get(p,28));maneuver_=p[32];
        if(progressGood_&&routeWindow_>=0&&fixState(now)==Fix::Precise&&timely(progressSample_,now)){
            const auto& w=slots_[routeWindow_].object;
            if(remaining_<=w.distance){const auto along=w.distance-remaining_;
                if(along<w.start||along>w.end||(w.end<w.distance&&std::uint64_t(along)+200>=w.end)){
                    routeNeedOutstanding_=true;
                    if(!routeNeedDirty_&&(!routeNeedGeneration_||requestedAlong_!=along)){requestedAlong_=along;if(!++routeNeedGeneration_)++routeNeedGeneration_;routeNeedDirty_=true;}
                }
            }
        }
    }
    void remember(const Message& m){last_=std::uint32_t(get(m.bytes.data(),24));history_[historyNext_]=m;historyNext_=(historyNext_+1)%4;historyCount_=std::min(4u,historyCount_+1);}
    void duplicate(const Message& m){
        for(unsigned i=0;i<historyCount_;++i)if(history_[i].size==m.size&&std::equal(m.bytes.begin(),m.bytes.begin()+m.size,history_[i].bytes.begin())){++metrics_.duplicates;dirty_=true;return;}
        error_=Error::Conflict;++metrics_.rejected;dirty_=true;
    }
    int id(unsigned object)const{for(unsigned i=0;i<cacheSlots;++i)if(slots_[i].valid&&slots_[i].id==object)return int(i);return -1;}
    int find(const Digest& hash,unsigned size)const{for(unsigned i=0;i<cacheSlots;++i)if(slots_[i].valid&&slots_[i].hash==hash&&slots_[i].size==size)return int(i);return -1;}
    int cell(unsigned z,unsigned x,unsigned y)const{for(unsigned i=0;i<cacheSlots;++i){const auto& s=slots_[i];if(s.valid&&s.object.kind==Kind::Cell&&s.object.z==z&&s.object.x==x&&s.object.y==y)return int(i);}return -1;}
    bool completeCell(unsigned z,unsigned x,unsigned y)const{const int i=cell(z,x,y);return i>=0&&!slots_[i].object.partial;}
    bool visible(const Object& o)const{
        if(!published_)return true;
        const auto center=maps::tilePoint(o.z,o.x,o.y,2048,2048);const double half=maps::worldMetres/(1u<<o.z)/2;
        return std::abs(maps::wrapEast(center.east-cameraWorld_.east))<=half+400/scale_&&std::abs(center.north-cameraWorld_.north)<=half+150/scale_;
    }
    unsigned selectCells()const{
        struct Choice {unsigned index=0,rank=0;double distance=0;};std::array<Choice,cacheSlots> choices{};unsigned count=0;
        for(unsigned i=0;i<cacheSlots;++i){const auto& s=slots_[i];const auto& o=s.object;if(!s.valid||o.kind!=Kind::Cell||!visible(o))continue;
            // An available target-LOD parent retires obsolete fine raw tiles.
            // The old immutable scene remains intact until the next swap.
            if(o.z>interestZ_&&completeCell(interestZ_,o.x>>(o.z-interestZ_),o.y>>(o.z-interestZ_)))continue;
            bool needed=true;
            if(o.z<interestZ_&&interestCount_){
                needed=false;for(unsigned k=0;k<interestCount_;++k)if((interestX_[k]>>(interestZ_-o.z))==o.x&&(interestY_[k]>>(interestZ_-o.z))==o.y&&!completeCell(interestZ_,interestX_[k],interestY_[k]))needed=true;
            }
            if(!needed)continue;
            const auto center=maps::tilePoint(o.z,o.x,o.y,2048,2048);const double x=maps::wrapEast(center.east-cameraWorld_.east),y=center.north-cameraWorld_.north;
            choices[count++]={i,o.z==interestZ_?0u:o.z<interestZ_?1u:2u,x*x+y*y};
        }
        std::sort(choices.begin(),choices.begin()+count,[](const auto& a,const auto& b){return a.rank!=b.rank?a.rank<b.rank:a.distance<b.distance;});
        unsigned mask=0;for(unsigned i=0;i<std::min(6u,count);++i)mask|=1u<<choices[i].index;return mask;
    }
    bool pinned(unsigned i)const{
        return (building_&&(buildMask_&(1u<<i)))||int(i)==overview_||int(i)==routeWindow_||
            int(i)==pendingOverview_||int(i)==pendingWindow_||
            (selectedMask_&(1u<<i));
    }
    bool routeCandidate(const Object& o){
        if(o.kind==Kind::Cell)return true;
        if(route_&&!(o.route==route_&&o.revision==revision_)&&!maplink::forward(o.revision,revision_)){error_=Error::Session;return false;}
        if(pendingRoute_){
            if(o.revision==pendingRevision_&&o.route!=pendingRoute_){error_=Error::Conflict;return false;}
            if(o.revision!=pendingRevision_&&!maplink::forward(o.revision,pendingRevision_)){error_=Error::Session;return false;}
            if(o.revision!=pendingRevision_)pendingOverview_=pendingWindow_=-1;
        }
        pendingRoute_=o.route;pendingRevision_=o.revision;
        if(o.route!=route_||o.revision!=revision_)progressGood_=false;
        return true;
    }
    void bindCandidate(int i){if(slots_[i].object.kind==Kind::Overview)pendingOverview_=i;else if(slots_[i].object.kind==Kind::RouteWindow)pendingWindow_=i;}
    void fail(Error e){error_=e;stage_=active_?Stage::Idle:Stage::Closed;dirty_=true;}
    void verify(std::uint64_t now){
        if(hashOffset_<total_){
            const unsigned end=std::min(total_,hashOffset_+512);sha_.update(staging_+hashOffset_,end-hashOffset_);
            for(unsigned i=std::max(hashOffset_,64u);i<end;++i){crc_^=staging_[i];for(unsigned j=0;j<8;++j)crc_=(crc_>>1)^((crc_&1)?0xedb88320u:0);}
            hashOffset_=end;if(end==total_&&(sha_.finish()!=expected_||(crc_^0xffffffffu)!=get(staging_,56))){++metrics_.hashFailures;fail(Error::Hash);}return;
        }
        if(!headerChecked_){
            if(!objectHeader(staging_,total_,candidate_)||candidate_.kind!=expectedKind_||candidate_.attribution!=attribution_||candidate_.source!=get(source_.data(),0)){fail(Error::Bounds);return;}headerChecked_=true;
        }
        for(unsigned budget=0;budget<16&&verifyIndex_<candidate_.count;++budget,++verifyIndex_){
            if(candidate_.kind==Kind::Cell){
                if(verifyOffset_>total_||total_-verifyOffset_<4){fail(Error::Bounds);return;}
                const auto* p=staging_+verifyOffset_;const unsigned n=p[1],name=p[2],length=4+name+n*4;
                if(p[0]>4||n<2||n>8||name>23||p[3]||length>total_-verifyOffset_){fail(Error::Bounds);return;}
                bool named=!name,distinct=false;
                for(unsigned i=0;i<name;++i){if(p[4+i]<32||p[4+i]>126){fail(Error::Bounds);return;}named|=p[4+i]!=32;}
                if(!named){fail(Error::Bounds);return;}
                for(unsigned i=0;i<n;++i){const auto* v=p+4+name+4*i;if(get(v,0,2)>4096||get(v,2,2)>4096){fail(Error::Bounds);return;}distinct|=get(v,0)!=get(p+4+name,0);}
                if(!distinct){fail(Error::Bounds);return;}verifyOffset_+=length;
            }else{
                const auto* p=staging_+64+verifyIndex_*8;const auto lat=signed32(p,0),lon=signed32(p,4);
                if(lat< -850511287||lat>850511287||lon< -1800000000||lon>1800000000){fail(Error::Bounds);return;}verifyOffset_+=8;
            }
        }
        if(verifyIndex_<candidate_.count)return;
        if(verifyOffset_!=total_){fail(Error::Bounds);return;}
        if(!routeCandidate(candidate_)){fail(error_);return;}
        int target=-1;
        for(unsigned i=0;i<cacheSlots;++i)if(slots_[i].valid&&candidate_.kind==Kind::Cell&&slots_[i].object.kind==Kind::Cell&&slots_[i].object.z==candidate_.z&&slots_[i].object.x==candidate_.x&&slots_[i].object.y==candidate_.y){target=int(i);break;}
        if(target>=0&&candidate_.partial&&!slots_[target].object.partial){fail(Error::Conflict);return;}
        if(target<0)for(unsigned i=0;i<cacheSlots;++i)if(!slots_[i].valid){target=int(i);break;}
        if(target<0)for(unsigned i=0;i<cacheSlots;++i)if(!pinned(i)&&(target<0||slots_[i].used<slots_[target].used))target=int(i);
        if(target<0){fail(Error::Resource);return;}
        if(building_&&(buildMask_&(1u<<target)))building_=false;
        auto& s=slots_[target];if(s.valid)++metrics_.evicted;
        std::swap(s.bytes,staging_);s.object=candidate_;s.hash=expected_;s.id=incoming_;s.size=total_;s.used=now;s.valid=true;
        bindCandidate(target);
        selectedMask_=selectCells();
        stage_=Stage::Ready;++metrics_.verified;renderDirty_=dirty_=interestDirty_=true;
    }
    void activate(const std::uint8_t* p,std::uint64_t now){
        const auto route=get(p,0,8);const auto rev=std::uint32_t(get(p,8));const int o=id(unsigned(get(p,12))),w=id(unsigned(get(p,16))),c=id(unsigned(get(p,20)));
        if((route_&&!(route==route_&&rev==revision_)&&!maplink::forward(rev,revision_))||(pendingRoute_&&((rev==pendingRevision_&&route!=pendingRoute_)||(rev!=pendingRevision_&&!maplink::forward(rev,pendingRevision_))))){error_=Error::Session;return;}
        if(o<0||w<0||c<0||slots_[o].object.kind!=Kind::Overview||slots_[w].object.kind!=Kind::RouteWindow||slots_[c].object.kind!=Kind::Cell||
           slots_[o].object.route!=route||slots_[w].object.route!=route||slots_[o].object.revision!=rev||slots_[w].object.revision!=rev){error_=Error::NotReady;return;}
        const auto target=fixState(now)==Fix::Precise||fixState(now)==Fix::Approximate?maps::mercator(lat_,lon_):cameraWorld_;
        const auto& cell=slots_[c].object;const auto center=maps::tilePoint(cell.z,cell.x,cell.y,2048,2048);const double half=maps::worldMetres/(1u<<cell.z)/2;
        if(std::abs(maps::wrapEast(target.east-center.east))>half+0.01||std::abs(target.north-center.north)>half+0.01){error_=Error::NotReady;return;}
        route_=route;revision_=rev;overview_=o;routeWindow_=w;pendingRoute_=0;pendingRevision_=0;pendingOverview_=pendingWindow_=-1;progressGood_=false;progressSample_=0;routeNeedOutstanding_=routeNeedDirty_=false;renderDirty_=true;
    }
    void startRender(){
        renderDirty_=false;building_=true;buildMask_=0;buildSlot_=1-renderSlot_;buildCell_=buildRoad_=buildOffset_=buildRoute_=0;
        renderClipped_=false;
        auto& out=*scenes_[buildSlot_];out.roadCount=out.routeCount=out.overviewCount=0;out.quality=Quality::Unavailable;out.mercator=true;
        auto center=cameraWorld_;
        if(!published_){bool found=fix_;if(fix_)center=maps::mercator(lat_,lon_);else for(const auto& s:slots_)if(s.valid&&s.object.kind==Kind::Cell){center=maps::tilePoint(s.object.z,s.object.x,s.object.y,2048,2048);found=true;break;}
            if(!found){building_=false;return;}cameraWorld_=center;}
        out.originLatE7=maps::latitudeE7(center.north);out.originLonE7=maps::longitudeE7(center.east);
        selectedMask_=selectCells();buildMask_=selectedMask_;buildCount_=0;
        for(unsigned i=0;i<cacheSlots;++i)if(buildMask_&(1u<<i))buildOrder_[buildCount_++]=i;
        std::sort(buildOrder_.begin(),buildOrder_.begin()+buildCount_,[&](unsigned a,unsigned b){return slots_[a].object.z<slots_[b].object.z;});
    }
    void renderStep(){
        auto& out=*scenes_[buildSlot_];unsigned budget=16;
        while(buildCell_<buildCount_&&budget){
            const auto& s=slots_[buildOrder_[buildCell_]];
            if(buildRoad_>=s.object.count){++buildCell_;buildRoad_=0;buildOffset_=64;continue;}
            if(!buildOffset_)buildOffset_=64;
            const auto* p=s.bytes+buildOffset_;const unsigned n=p[1],name=p[2];buildOffset_+=4+name+n*4;++buildRoad_;--budget;
            if(out.roadCount>=out.roads.size()){renderClipped_=true;coverage_=Coverage::Partial;continue;}
            auto& road=out.roads[out.roadCount++];road.kind=maps::RoadKind(p[0]);road.count=n;road.name.fill(0);std::copy_n(p+4,name,road.name.begin());
            for(unsigned i=0;i<n;++i)road.points[i]=project(maps::tilePoint(s.object.z,s.object.x,s.object.y,unsigned(get(p,4+name+i*4,2)),unsigned(get(p,6+name+i*4,2))),out);
        }
        if(buildCell_<buildCount_)return;
        const unsigned count=std::max(overview_>=0?slots_[overview_].object.count:0,routeWindow_>=0?slots_[routeWindow_].object.count:0);
        for(unsigned i=0;i<32&&buildRoute_<count;++i,++buildRoute_){
            const auto copy=[&](int slot,auto& points,unsigned& n){if(slot<0||buildRoute_>=slots_[slot].object.count)return;const auto* p=slots_[slot].bytes+64+buildRoute_*8;points[n++]=project(maps::mercator(signed32(p,0),signed32(p,4)),out);};
            copy(overview_,out.routeOverview,out.overviewCount);copy(routeWindow_,out.route,out.routeCount);
        }
        if(buildRoute_<count)return;
        out.quality=Quality::Available;renderSlot_=buildSlot_;published_=true;building_=false;dirty_=true;
    }
    std::array<Slot,cacheSlots> slots_{};std::uint8_t* staging_=nullptr;maps::Scene* scenes_[2]{};
    std::array<Message,4> history_{};unsigned historyNext_=0,historyCount_=0;
    Digest source_{},expected_{};Object candidate_;Kind expectedKind_=Kind::Cell;maplink::Sha256 sha_;Metrics metrics_;
    bool renderClipped_=false;
    bool ready_=false,link_=false,active_=false,dirty_=true,renderDirty_=false,building_=false,published_=false,headerChecked_=false;
    std::uint32_t boot_=1,vehicle_=0,connection_=0,nav_=0,maxNav_=0,last_=0,incoming_=0,locSequence_=0,progressSequence_=0,revision_=0,pendingRevision_=0;
    unsigned window_=4,attribution_=1,offset_=0,total_=0,hashOffset_=0,verifyIndex_=0,verifyOffset_=64;std::uint32_t crc_=0;
    Stage stage_=Stage::Closed;Error error_=Error::None;Fix reportedFix_=Fix::None,lastFix_=Fix::None;Coverage coverage_=Coverage::Missing;
    std::uint64_t route_=0,pendingRoute_=0,lastData_=0,t0_=0,db_=0,phoneBase_=0,anchorAt_=0,uncertainty_=2,sample_=0,dispatch_=0,fixAt_=0,progressSample_=0,progressAt_=0;
    std::uint32_t timeSequence_=0,remaining_=0,remainingSeconds_=0,maneuverDistance_=0;unsigned maneuver_=0;
    bool anchor_=false,pendingTime_=false,fix_=false,seenFix_=false,knownFix_=false,progressGood_=false,following_=true;std::int32_t lat_=0,lon_=0;
    unsigned accuracy_=UINT32_MAX,permission_=0,course_=65535,courseAccuracy_=65535,speed_=65535;
    int overview_=-1,routeWindow_=-1,pendingOverview_=-1,pendingWindow_=-1;unsigned renderSlot_=0,buildSlot_=1,buildMask_=0,requestedMask_=0,selectedMask_=0,buildCount_=0,buildCell_=0,buildRoad_=0,buildOffset_=64,buildRoute_=0;std::array<unsigned,6> buildOrder_{};
    maps::WorldPoint cameraWorld_{};double scale_=0.48;std::array<std::uint32_t,4> interestX_{},interestY_{};unsigned interestZ_=14,interestCount_=0,haveMask_=0;std::uint32_t interestGeneration_=0,routeNeedGeneration_=0,requestedAlong_=0;std::uint64_t lastInterestReport_=0,lastRouteNeedReport_=0;bool interestDirty_=false,routeNeedDirty_=false,routeNeedOutstanding_=false;
};
}
