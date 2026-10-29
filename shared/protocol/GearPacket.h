#pragma once
#include "shared/model/ClusterState.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace cluster::gearlink {
// Development discrete-input transport. Explicit little-endian bytes, never native structs.
constexpr std::size_t packetSize=36;
using Bytes=std::array<std::uint8_t,packetSize>;
struct Packet {
    std::uint32_t session=0,sequence=0,uptimeMs=0;
    std::uint8_t levels=0;
    std::uint32_t tripPresses=0,infoPresses=0;
    Signal<int> gear;
    Signal<bool> left,right,highBeam;
};
inline std::uint32_t crc(const std::uint8_t* data,std::size_t size) {
    std::uint32_t result=0xffffffffu;
    for(std::size_t i=0;i<size;++i) {
        result^=data[i];
        for(int bit=0;bit<8;++bit) result=(result>>1)^((result&1)?0xedb88320u:0u);
    }
    return result^0xffffffffu;
}
inline void put(Bytes& b,std::size_t offset,std::uint32_t n) {
    for(int i=0;i<4;++i) b[offset+i]=std::uint8_t(n>>(8*i));
}
inline std::uint32_t get(const Bytes& b,std::size_t offset) {
    std::uint32_t n=0;for(int i=0;i<4;++i)n|=std::uint32_t(b[offset+i])<<(8*i);return n;
}
inline Bytes encode(const Packet& p) {
    Bytes b{};b[0]='R';b[1]='1';b[2]='5';b[3]='G';b[4]=3;
    b[5]=p.gear.quality==Quality::Available?1:p.gear.quality==Quality::Invalid?2:0;
    b[6]=p.gear.available()?std::uint8_t(p.gear.value):0;b[7]=p.levels;
    put(b,8,p.session);put(b,12,p.sequence);put(b,16,p.uptimeMs);const Signal<bool> lamps[]={p.left,p.right,p.highBeam};
    for(int i=0;i<3;++i)b[20+i]=lamps[i].available()?(lamps[i].value?2:1):0;
    put(b,24,p.tripPresses);put(b,28,p.infoPresses);
    put(b,32,crc(b.data(),32));return b;
}
inline bool decode(const Bytes& b,Packet& result) {
    if(b[0]!='R'||b[1]!='1'||b[2]!='5'||b[3]!='G'||b[4]!=3||b[5]>2||b[7]>3||b[20]>2||b[21]>2||b[22]>2||b[23]!=0||get(b,32)!=crc(b.data(),32))return false;
    if((b[5]==1&&b[6]>6)||(b[5]!=1&&b[6]!=0))return false;
    Packet p;p.session=get(b,8);p.sequence=get(b,12);p.uptimeMs=get(b,16);p.levels=b[7];
    p.gear={b[6],b[5]==1?Quality::Available:b[5]==2?Quality::Invalid:Quality::Unavailable};p.left={b[20]==2,b[20]?Quality::Available:Quality::Unavailable};
    p.right={b[21]==2,b[21]?Quality::Available:Quality::Unavailable};
    p.tripPresses=get(b,24);p.infoPresses=get(b,28);
    p.highBeam={b[22]==2,b[22]?Quality::Available:Quality::Unavailable};result=p;return true;
}
struct ButtonClicks {unsigned trip=0,info=0;};
class Receiver {
    Bytes buffer_{};std::size_t size_=0;
    ButtonClicks pending_{};
    Packet latest_{};bool received_=false;std::uint64_t receipt_=0;
public:
    bool received()const{return received_;}
    bool fresh(std::uint64_t now)const{return received_&&now>=receipt_&&now-receipt_<1000;}
    ButtonClicks takeClicks(){auto result=pending_;pending_={};return result;}
    void reset(){pending_={};size_=0;received_=false;latest_={};receipt_=0;}
    bool push(std::uint8_t byte,std::uint64_t now) {
        buffer_[size_++]=byte;
        if(size_<packetSize)return false;
        Packet p;
        if(!decode(buffer_,p)) {
            for(std::size_t i=1;i<packetSize;++i)buffer_[i-1]=buffer_[i];
            --size_;return false;
        }
        size_=0;
        if(received_&&p.session==latest_.session) {
            const auto step=p.sequence-latest_.sequence,elapsed=p.uptimeMs-latest_.uptimeMs;
            if(step==0||step>=0x80000000u||elapsed==0||elapsed>=0x80000000u)return false;
        }
        if(received_&&p.session==latest_.session&&now-receipt_<1000) {
            const auto add=[](unsigned pending,std::uint32_t delta) {
                return delta<=8&&pending+delta<=8?unsigned(pending+delta):0u;
            };
            pending_.trip=add(pending_.trip,p.tripPresses-latest_.tripPresses);
            pending_.info=add(pending_.info,p.infoPresses-latest_.infoPresses);
        } else pending_={}; // Baseline on connect/reset/stale; never replay old clicks.
        latest_=p;received_=true;receipt_=now;return true;
    }
    Signal<int> gear(std::uint64_t now)const {
        if(!received_)return {};
        if(now-receipt_>=1000)return {0,Quality::Stale};
        return latest_.gear;
    }
    ClusterState state(std::uint64_t now)const {
        ClusterState s;s.turnSignalsArePhase=true;s.gear=gear(now);
        if(received_) {s.left=latest_.left;s.right=latest_.right;s.highBeam=latest_.highBeam;
            if(now-receipt_>=1000)s.left.quality=s.right.quality=s.highBeam.quality=Quality::Stale;
        }
        return s;
    }
    const Packet& latest()const{return latest_;}
};
} // namespace cluster::gearlink
