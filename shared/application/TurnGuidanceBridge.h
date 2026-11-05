#pragma once
#include "shared/protocol/TurnGuidance.h"
namespace cluster::guidance {
// Vehicle ingress/relay. Host/app tasks serialize access in the board adapter.
// ATT admission never invents a display ACK or renews source timing.
class Bridge {
public:
    void session(std::uint32_t boot,std::uint32_t connection,bool live,std::uint64_t now){
        if(boot!=boot_||connection!=connection_){*this=Bridge{};boot_=boot;connection_=connection;resetTx_=true;}
        if(live_&&!live){pending_=false;capAt_=statusAt_=0;awaitingAck_=false;requiresOpen_=true;resetTx_=true;}
        live_=live;
        if(capAt_&&(now<capAt_||now-capAt_>=2000)){pending_=false;requiresOpen_=true;awaitingAck_=false;resetTx_=true;}
    }
    void downstream(const Message& m,std::uint64_t now){
        const auto* b=m.bytes.data();
        if(get(b,8)!=boot_||get(b,12)!=connection_)return;
        if(validCapability(m)){
            const auto display=std::uint32_t(get(b,16));
            if(display!=display_){display_=display;sequence_=stream_=0;last_={};pending_=false;status_={};statusAt_=0;resetTx_=true;}
            std::copy_n(b,80,cap_.begin());capAt_=now;
            if(!(b[7]&1)){pending_=false;awaitingAck_=false;requiresOpen_=true;resetTx_=true;}
            return;
        }
        if(!validStatus(m)||get(b,16)!=display_)return;
        const auto consumed=std::uint32_t(get(b,24));
        if(get(b,20)!=stream_||(consumed!=sequence_&&(!sequence_||!forward(sequence_,consumed))))return;
        // Never replace a newer display observation with an old UART retry.
        if(statusAt_&&get(b,40,8)<get(status_.data(),40,8))return;
        std::copy_n(b,112,status_.begin());statusAt_=now;dirty_=true;
        if(get(b,96)==sequence_&&b[34])awaitingAck_=false;
        if(awaitingAck_&&consumed==sequence_&&(last_.bytes[5]!=std::uint8_t(Command::Snapshot)||(b[7]&4))&&now>=admittedAt_){maxAckMs_=std::max(maxAckMs_,unsigned(std::min<std::uint64_t>(now-admittedAt_,unknown-1)));awaitingAck_=false;}
    }
    bool ready(std::uint64_t now)const{return live_&&boot_&&connection_&&display_&&capAt_&&now>=capAt_&&now-capAt_<2000&&(cap_[7]&1);}
    Capability capability(std::uint64_t now,unsigned mtu)const{
        auto b=capabilities(boot_,connection_,display_,ready(now)&&mtu>=minimumMtu,capAt_?get(cap_.data(),56,8):0,mtu);return b;
    }
    Status status(std::uint64_t now)const{
        if(ready(now)&&statusAt_&&now>=statusAt_&&now-statusAt_<2000)return status_;
        Status b{};header(b.data(),'W',b.size());put(b.data(),8,boot_);put(b.data(),12,connection_);put(b.data(),16,display_);put(b.data(),20,stream_);b[33]=std::uint8_t(Quality::LinkLost);b[35]=std::uint8_t(Reason::LinkLost);put(b.data(),36,unknown);put(b.data(),80,unknownTime,8);put(b.data(),104,unknown);seal(b.data(),b.size());return b;
    }
    Error admit(const Message& m,std::uint64_t now){
        const auto fail=[&](Error e){++rejected_;return e;};
        if(!validCommand(m))return fail(Error::Invalid);
        const auto* b=m.bytes.data();const auto seq=std::uint32_t(get(b,24)),stream=std::uint32_t(get(b,20));const auto type=Command(b[5]);
        if(!ready(now)||get(b,8)!=boot_||get(b,12)!=connection_||get(b,16)!=display_)return fail(Error::Context);
        if(seq==sequence_){
            if(last_.size!=m.size||!std::equal(b,b+m.size,last_.bytes.begin()))return fail(Error::Sequence);
            ++duplicates_;if(now>=sentAt_&&now-sentAt_>=500){pending_=true;sentAt_=now;}return Error::None;
        }
        if(sequence_&&!forward(seq,sequence_))return fail(Error::Sequence);
        if(type==Command::Open){if(stream==stream_)return fail(Error::Sequence);}
        else if(stream!=stream_)return fail(Error::Context);
        else if(requiresOpen_&&type!=Command::Clear)return fail(Error::Closed);
        const bool outstanding=awaitingAck_;
        if(outstanding&&type!=Command::Clear)return fail(Error::Busy);
        if(type==Command::Snapshot&&snapshotAt_&&now>=snapshotAt_&&now-snapshotAt_<500)return fail(Error::Rate);
        if(type==Command::Presence&&presenceAt_&&now>=presenceAt_&&now-presenceAt_<2000)return fail(Error::Rate);
        if(type==Command::Snapshot)snapshotAt_=now;
        if(type==Command::Presence)presenceAt_=now;
        if(type==Command::Open){stream_=stream;statusAt_=0;requiresOpen_=false;}
        if(type==Command::Clear)requiresOpen_=true;
        sequence_=seq;last_=m;pending_=true;awaitingAck_=true;sentAt_=admittedAt_=now;++accepted_;return Error::None;
    }
    bool take(Message& m){if(!pending_)return false;m=last_;pending_=false;return true;}
    bool takeReset(){const auto v=resetTx_;resetTx_=false;return v;}
    bool takeDirty(){const auto v=dirty_;dirty_=false;return v;}
    struct Diagnostics {unsigned accepted,rejected,duplicates,maxAckMs;};
    Diagnostics diagnostics()const{return {accepted_,rejected_,duplicates_,maxAckMs_};}
private:
    Capability cap_{};Status status_{};Message last_;
    std::uint32_t boot_=0,connection_=0,display_=0,sequence_=0,stream_=0;
    std::uint64_t capAt_=0,statusAt_=0,admittedAt_=0,sentAt_=0,snapshotAt_=0,presenceAt_=0;
    bool live_=false,pending_=false,dirty_=false,resetTx_=false,awaitingAck_=false,requiresOpen_=true;
    unsigned accepted_=0,rejected_=0,duplicates_=0,maxAckMs_=0;
};
static_assert(sizeof(Bridge)<900,"Guidance vehicle bridge budget");
}
