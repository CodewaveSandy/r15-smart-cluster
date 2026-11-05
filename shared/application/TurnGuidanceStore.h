#pragma once
#include "shared/protocol/TurnGuidance.h"
namespace cluster::guidance {
// Display application owner: complete commands enter here, never the widget.
class Store {
public:
    void initialize(std::uint32_t display){display_=display?display:1;}
    void session(std::uint32_t boot,std::uint32_t connection,bool valid,std::uint64_t now){
        if(!valid||!boot||!connection){if(link_){close(Reason::LinkLost,now);link_=false;}return;}
        if(boot!=boot_||connection!=connection_){
            const auto display=display_;*this=Store{};display_=display;boot_=boot;connection_=connection;
        }
        link_=true;poll(now);
    }
    void invalidate(std::uint64_t now){close(Reason::LinkLost,now);link_=false;}
    bool consume(const Message& m,std::uint64_t now){
        if(!validCommand(m))return reject(Error::Invalid,m,now);
        const auto* b=m.bytes.data();const auto seq=std::uint32_t(get(b,24)),stream=std::uint32_t(get(b,20));const auto type=Command(b[5]);
        if(!link_||get(b,8)!=boot_||get(b,12)!=connection_||get(b,16)!=display_)return reject(Error::Context,m,now);
        poll(now);
        if(seq==sequence_){
            if(last_.size==m.size&&std::equal(b,b+m.size,last_.bytes.begin())){++duplicates_;return true;}
            return reject(Error::Sequence,m,now);
        }
        if(sequence_&&!forward(seq,sequence_))return reject(Error::Sequence,m,now);
        if(type==Command::Open){
            if(stream==stream_)return reject(Error::Sequence,m,now);
            content_={};revision_=0;stream_=stream;open_=true;reason_=Reason::None;withdrawn_=false;anchor_={};request_={};observedAt_=now;arrivalAt_=0;
        }else {
            // A priority Stop may overtake an Open that has not reached this
            // display. Its forward global sequence fences both streams.
            if(type==Command::Clear)stream_=stream;
            else if(stream!=stream_)return reject(Error::Context,m,now);
            // Clear remains valid after expiry so a Stop tombstone is acknowledged.
            if(!open_&&type!=Command::Clear)return reject(Error::Closed,m,now);
            switch(type){
            case Command::TimeRequest:request_={seq,get(b,40,8),now};break;
            case Command::TimeConfirm:{
                if(get(b,40)!=request_.sequence||get(b,48,8)!=request_.t0||get(b,64,8)!=request_.db||!request_.sequence||now<request_.db||now-request_.db>1000)return reject(Error::Clock,m,now);
                if(clockValid(now)&&request_.db>=anchor_.db){
                    const auto elapsed=request_.db-anchor_.db,margin=2+(elapsed+9999)/10000;
                    const auto lower=anchor_.t0+elapsed>margin?anchor_.t0+elapsed-margin:0;
                    if(get(b,56,8)<lower||request_.t0>anchor_.t1+elapsed+margin)return reject(Error::Clock,m,now);
                }
                anchor_={request_.t0,get(b,56,8),request_.db,true};break;
            }
            case Command::Snapshot:{
                Snapshot next;decodeSnapshot(m,next);const auto rev=std::uint32_t(get(b,28));
                if(!clockValid(now)||next.dispatch>phoneUpper(now)||phoneUpper(now)-next.dispatch>=transportTtl)return reject(Error::Clock,m,now);
                if(revision_){
                    if(rev==revision_){if(next.dispatch<content_.dispatch||!sameCore(content_,next))return reject(Error::Revision,m,now);}
                    else if(!forward(rev,revision_)||sameContent(content_,next)||next.dispatch<content_.dispatch||(content_.changed!=unknownTime&&next.changed!=unknownTime&&next.changed<content_.changed))return reject(Error::Revision,m,now);
                }
                const bool newContent=rev!=revision_;
                if(newContent&&next.source==Source::Arrived)arrivalAt_=now;
                if(next.source!=Source::Arrived)arrivalAt_=0;
                content_=next;revision_=rev;withdrawn_=false;observedAt_=contentAt_=now;
                acceptedAge_=sourceAge(now);break;
            }
            case Command::Presence:
                if(get(b,28)!=revision_)return reject(Error::Revision,m,now);
                observedAt_=now;if(b[40]!=7)withdrawn_=true;
                break;
            case Command::Clear:close(Reason(b[40]),now);break;
            default:return reject(Error::Invalid,m,now);
            }
        }
        sequence_=seq;last_=m;receivedAt_=now;error_=Error::None;published_=false;actuallyVisible_=false;++accepted_;return true;
    }
    void poll(std::uint64_t now){if(open_&&now>=observedAt_&&now-observedAt_>=transportTtl)close(Reason::LinkLost,now);}
    TurnGuidanceState view(std::uint64_t now)const {
        TurnGuidanceState v;v.content=content_;v.revision=revision_;v.reason=reason_;v.clock=clockValid(now);
        v.ageMs=sourceAge(now);v.ageKnown=v.ageMs!=unknown;v.transport=open_&&link_&&now>=observedAt_&&now-observedAt_<transportTtl;
        if(!open_){
            v.quality=reason_==Reason::None?Quality::Inactive:reason_==Reason::LinkLost?Quality::LinkLost:Quality::Cleared;
            v.visible=reason_!=Reason::None&&reason_!=Reason::Stop&&now>=closedAt_&&now-closedAt_<3000;
            return v;
        }
        v.visible=true;
        if(!revision_){v.quality=Quality::Waiting;return v;}
        if(content_.source==Source::Arrived&&now>=arrivalAt_&&now-arrivalAt_>=5000){v.visible=false;v.quality=Quality::Inactive;return v;}
        if(content_.source==Source::Unsupported){v.quality=Quality::Unsupported;return v;}
        if(content_.source!=Source::Guiding&&content_.source!=Source::Arrived){v.quality=Quality::Waiting;return v;}
        if(withdrawn_){v.quality=Quality::Pending;return v;}
        if(!v.clock||!v.ageKnown){v.quality=acceptedAge_!=unknown&&now>=contentAt_&&std::uint64_t(acceptedAge_)+now-contentAt_>=hideAge?Quality::Pending:Quality::UnknownAge;return v;}
        if(v.ageMs>=hideAge){v.quality=Quality::Pending;return v;}
        if(v.ageMs>=dimAge){v.quality=Quality::LastUpdate;return v;}
        v.quality=Quality::Fresh;v.action=v.transport&&!(content_.flags&Truncated)&&(content_.flags&Direction);return v;
    }
    void published(std::uint64_t now,bool visible){if(!published_){published_=true;publishedAt_=now;}actuallyVisible_=visible;}
    Capability capability(std::uint64_t now)const{return capabilities(boot_,connection_,display_,link_,now);}
    Status status(std::uint64_t now)const{
        const auto v=view(now);Status b{};header(b.data(),'W',b.size());
        b[7]=(open_?1:0)|(v.clock?2:0)|(published_?4:0)|(v.action?8:0)|(v.transport?16:0)|(v.ageKnown?32:0)|(v.quality==Quality::LastUpdate?64:0)|(actuallyVisible_&&v.visible?128:0);
        put(b.data(),8,boot_);put(b.data(),12,connection_);put(b.data(),16,display_);put(b.data(),20,stream_);put(b.data(),24,sequence_);put(b.data(),28,revision_);
        b[32]=std::uint8_t(content_.source);b[33]=std::uint8_t(v.quality);b[34]=std::uint8_t(error_);b[35]=std::uint8_t(reason_);
        put(b.data(),36,v.ageMs);put(b.data(),40,receivedAt_,8);put(b.data(),48,published_?publishedAt_:0,8);put(b.data(),56,request_.sequence);put(b.data(),64,request_.t0,8);put(b.data(),72,request_.db,8);put(b.data(),80,content_.changed,8);put(b.data(),88,observedAt_,8);put(b.data(),96,rejectedSequence_);put(b.data(),100,rejected_);put(b.data(),104,acceptedAge_);seal(b.data(),b.size());return b;
    }
    struct Diagnostics {unsigned accepted,rejected,duplicates,expired;};
    Diagnostics diagnostics()const{return {accepted_,rejected_,duplicates_,expired_};}
private:
    struct Request {std::uint32_t sequence=0;std::uint64_t t0=0,db=0;}request_;
    struct Anchor {std::uint64_t t0=0,t1=0,db=0;bool valid=false;}anchor_;
    bool clockValid(std::uint64_t now)const{return anchor_.valid&&now>=anchor_.db&&now-anchor_.db<anchorTtl;}
    std::uint64_t phoneUpper(std::uint64_t now)const{const auto d=now-anchor_.db;return anchor_.t1+d+2+(d+9999)/10000;}
    std::uint32_t sourceAge(std::uint64_t now)const{
        if(!clockValid(now)||!(content_.flags&AgeKnown)||content_.changed==unknownTime)return unknown;
        const auto upper=phoneUpper(now);return upper>=content_.changed?std::uint32_t(std::min<std::uint64_t>(unknown-1,upper-content_.changed)):unknown;
    }
    void close(Reason reason,std::uint64_t now){if(open_&&reason==Reason::LinkLost)++expired_;open_=false;reason_=reason;closedAt_=now;content_={};revision_=0;anchor_={};request_={};withdrawn_=true;actuallyVisible_=false;acceptedAge_=unknown;}
    bool reject(Error e,const Message& m,std::uint64_t){error_=e;if(rejected_!=unknown)++rejected_;rejectedSequence_=m.size>=28?std::uint32_t(get(m.bytes.data(),24)):0;return false;}
    std::uint32_t boot_=0,connection_=0,display_=1,stream_=0,sequence_=0,revision_=0,rejectedSequence_=0,acceptedAge_=unknown;
    std::uint64_t observedAt_=0,contentAt_=0,receivedAt_=0,publishedAt_=0,arrivalAt_=0,closedAt_=0;
    bool link_=false,open_=false,withdrawn_=false,published_=false,actuallyVisible_=false;
    Reason reason_=Reason::None;Error error_=Error::None;Snapshot content_;Message last_;
    unsigned accepted_=0,rejected_=0,duplicates_=0,expired_=0;
};
static_assert(sizeof(Store)<1400,"Guidance display store budget");
}
