#pragma once
#include "shared/protocol/GearPacket.h"
#include <cstdio>
namespace cluster::phonelink {
constexpr std::size_t packetSize=48;
using Bytes=std::array<std::uint8_t,packetSize>;
enum class Kind:std::uint8_t {Status=1,Control=2};
enum class Action:std::uint8_t {Heartbeat,Pair,Approve,Reject,Disconnect,Forget};
enum class Result:std::uint8_t {None,Queued,Rejected};
struct Packet {
    Kind kind=Kind::Status;Action action=Action::Heartbeat;Result result=Result::None;
    std::uint32_t vehicle=0,display=0,sequence=0,request=0,pairing=0,code=0,uptime=0;
    PhoneState phone;
};
inline void put(Bytes& b,unsigned offset,std::uint32_t value){for(unsigned i=0;i<4;++i)b[offset+i]=std::uint8_t(value>>(8*i));}
inline std::uint32_t get(const Bytes& b,unsigned offset){std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::uint32_t(b[offset+i])<<(8*i);return value;}
inline Bytes encode(const Packet& p) {
    Bytes b{};b[0]='R';b[1]='1';b[2]='5';b[3]='P';b[4]=1;b[5]=std::uint8_t(p.kind);
    if(p.kind==Kind::Status&&p.phone.available())b[6]=std::uint8_t(p.phone.paired|(p.phone.connected<<1)|(p.phone.pairingRequested<<2)|(p.phone.pairingApproved<<3)|(p.phone.pairWindowOpen<<4)|(p.phone.commandFailed<<5));
    b[7]=std::uint8_t(p.result);put(b,8,p.vehicle);put(b,12,p.display);put(b,16,p.sequence);put(b,20,p.request);
    put(b,24,p.pairing);put(b,28,p.code);b[32]=std::uint8_t(p.action);
    b[33]=p.kind==Kind::Status?std::uint8_t(p.phone.quality):0;put(b,36,p.uptime);
    put(b,44,gearlink::crc(b.data(),44));return b;
}
inline bool decode(const Bytes& b,Packet& result) {
    if(b[0]!='R'||b[1]!='1'||b[2]!='5'||b[3]!='P'||b[4]!=1||(b[5]!=1&&b[5]!=2)||b[6]>63||b[7]>2||b[32]>5||b[34]||b[35]||get(b,40)||get(b,44)!=gearlink::crc(b.data(),44))return false;
    Packet p;p.kind=Kind(b[5]);p.result=Result(b[7]);p.action=Action(b[32]);
    p.vehicle=get(b,8);p.display=get(b,12);p.sequence=get(b,16);p.request=get(b,20);p.pairing=get(b,24);p.code=get(b,28);p.uptime=get(b,36);
    if(!p.vehicle)return false;
    if(p.kind==Kind::Control) {
        if(!p.display||b[6]||b[7]||b[33]||p.code||(p.action==Action::Heartbeat?p.request!=0:p.request==0))return false;
        if((p.action==Action::Approve||p.action==Action::Reject)?p.pairing==0:p.pairing!=0)return false;
    } else {
        if(p.result==Result::None?(p.request||p.action!=Action::Heartbeat):(!p.display||!p.request||p.action==Action::Heartbeat))return false;
        if(b[33]!=unsigned(Quality::Available)&&b[33]!=unsigned(Quality::Unavailable)&&b[33]!=unsigned(Quality::Stale))return false;
        p.phone.quality=Quality(b[33]);p.phone.bleBench=true;
        if(!p.phone.available()&&(b[6]||p.pairing||p.code))return false;
        p.phone.paired=b[6]&1;p.phone.connected=b[6]&2;p.phone.pairingRequested=b[6]&4;p.phone.pairingApproved=b[6]&8;
        p.phone.pairWindowOpen=b[6]&16;p.phone.commandFailed=b[6]&32;
        if(p.phone.pairingRequested) {if(!p.pairing||p.code>999999)return false;std::snprintf(p.phone.pairingCode.data(),7,"%06u",unsigned(p.code));}
        else if(p.pairing||p.code)return false;
        if(!valid(p.phone)||(p.phone.paired&&p.phone.pairWindowOpen))return false;
    }
    result=p;return true;
}
class Receiver {
    Bytes bytes_{};std::size_t size_=0;Packet last_;bool seen_=false;std::uint64_t receipt_=0;
public:
    void reset(){size_=0;seen_=false;last_={};receipt_=0;}
    bool push(std::uint8_t byte,std::uint64_t now,Kind wanted) {
        bytes_[size_++]=byte;if(size_<packetSize)return false;
        Packet p;if(!decode(bytes_,p)) {for(unsigned i=1;i<packetSize;++i)bytes_[i-1]=bytes_[i];--size_;return false;}
        size_=0;if(p.kind!=wanted)return false;
        const auto owner=wanted==Kind::Status?p.vehicle:p.display;
        const auto previous=wanted==Kind::Status?last_.vehicle:last_.display;
        if(seen_&&owner==previous) {
            const auto step=p.sequence-last_.sequence,elapsed=p.uptime-last_.uptime;
            if(!step||step>=0x80000000u||!elapsed||elapsed>=0x80000000u)return false;
        }
        last_=p;seen_=true;receipt_=now;return true;
    }
    bool fresh(std::uint64_t now)const{return seen_&&now>=receipt_&&now-receipt_<1000;}
    const Packet& latest()const{return last_;}
};
struct Request {Action action=Action::Heartbeat;std::uint32_t pairing=0;};
class Gate {
    std::uint32_t vehicle_,display_=0,lastId_=0;std::uint64_t receipt_=0;bool heard_=false;
    Action lastAction_=Action::Heartbeat;Result result_=Result::None;
public:
    explicit Gate(std::uint32_t vehicle):vehicle_(vehicle){}
    bool fresh(std::uint64_t now)const{return heard_&&now>=receipt_&&now-receipt_<1000;}
    void invalidate(){heard_=false;}
    void reset(){display_=lastId_=0;heard_=false;lastAction_=Action::Heartbeat;result_=Result::None;}
    // Only a targeted heartbeat establishes a new display session. Commands
    // never execute twice and numeric approval is bound to the pending pairing.
    bool receive(const Packet& p,std::uint64_t now,std::uint32_t pairing,Request& request,bool& newDisplay) {
        newDisplay=false;if(p.vehicle!=vehicle_)return false;
        if(p.action==Action::Heartbeat) {
            if(display_!=p.display){reset();display_=p.display;newDisplay=true;}
            heard_=true;receipt_=now;return false;
        }
        if(p.display!=display_||!fresh(now))return false;
        receipt_=now;
        if(p.request==lastId_)return false;
        const auto step=p.request-lastId_;if(lastId_&&step>=0x80000000u)return false;
        lastId_=p.request;lastAction_=p.action;result_=Result::Rejected;
        if((p.action==Action::Approve||p.action==Action::Reject)&&(!pairing||p.pairing!=pairing))return false;
        request={p.action,p.pairing};return true;
    }
    void queued(bool accepted){result_=accepted?Result::Queued:Result::Rejected;}
    void stamp(Packet& p)const{p.display=display_;p.request=lastId_;p.action=lastAction_;p.result=result_;}
};
class Client {
    std::uint32_t own_=0,sequence_=0,nextId_=0;Packet status_,pending_;bool seen_=false,pendingActive_=false,failed_=false,forgetting_=false;
    std::uint64_t receipt_=0,nextSend_=0,started_=0;
public:
    void initialize(std::uint32_t session){*this=Client{};own_=session?session:1;}
    void update(const Packet& p,std::uint64_t now) {
        if(seen_&&status_.vehicle!=p.vehicle){failed_=pendingActive_||forgetting_;pendingActive_=forgetting_=false;nextSend_=0;}
        status_=p;seen_=true;receipt_=now;
        if(pendingActive_&&p.display==own_&&p.request==pending_.request&&p.result!=Result::None){failed_=p.result==Result::Rejected;pendingActive_=false;if(failed_)forgetting_=false;}
        if(forgetting_&&p.display==own_&&(!p.phone.paired||p.phone.commandFailed)){forgetting_=false;}
    }
    bool fresh(std::uint64_t now)const{return seen_&&now>=receipt_&&now-receipt_<1000;}
    PhoneState phone(std::uint64_t now,std::uint32_t gearSession)const {
        PhoneState result;result.bleBench=true;
        if(!fresh(now)){result.unavailable=PhoneUnavailable::Status;return result;}
        if(status_.vehicle!=gearSession){result.unavailable=PhoneUnavailable::VehicleSession;return result;}
        if(status_.display!=own_){result.unavailable=PhoneUnavailable::ReturnChannel;return result;}
        result=status_.phone;result.forgetting=forgetting_;result.commandFailed=result.commandFailed||failed_;return result;
    }
    bool post(Action action,std::uint64_t now,std::uint32_t gearSession) {
        const auto state=phone(now,gearSession);
        if(pendingActive_||forgetting_)return false;
        if(!state.available()||action==Action::Heartbeat){failed_=true;return false;}
        if((action==Action::Approve||action==Action::Reject)&&!state.pairingRequested){failed_=true;return false;}
        pending_={};pending_.action=action;pending_.request=++nextId_;if(!nextId_)pending_.request=++nextId_;
        if(action==Action::Approve||action==Action::Reject)pending_.pairing=status_.pairing;
        pendingActive_=true;forgetting_=action==Action::Forget;failed_=false;started_=now;nextSend_=0;return true;
    }
    bool outgoing(std::uint64_t now,Packet& out) {
        if(pendingActive_&&now-started_>=1500){pendingActive_=forgetting_=false;failed_=true;}
        if(forgetting_&&now-started_>=3500){forgetting_=false;failed_=true;}
        if(!fresh(now)||now<nextSend_)return false;
        out=pendingActive_?pending_:Packet{};out.kind=Kind::Control;out.vehicle=status_.vehicle;out.display=own_;
        out.sequence=++sequence_;out.uptime=std::uint32_t(now);nextSend_=now+200;return true;
    }
    void reset(){const auto own=own_,sequence=sequence_,id=nextId_;const bool uncertain=pendingActive_||forgetting_;initialize(own);sequence_=sequence;nextId_=id;failed_=uncertain;}
};
}
