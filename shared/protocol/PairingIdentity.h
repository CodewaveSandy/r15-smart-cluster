#pragma once
#include "shared/protocol/GearPacket.h"
#include <algorithm>
namespace cluster::pairidentity {
constexpr unsigned packetSize=64;
using Name=std::array<char,33>;
inline bool name(const std::uint8_t* bytes,unsigned size,Name& out) {
    if(!size||size>32)return false;
    bool visible=false;Name parsed{};
    for(unsigned i=0;i<size;++i){if(bytes[i]<32||bytes[i]>126)return false;parsed[i]=char(bytes[i]);visible|=bytes[i]!=32;}
    if(!visible)return false;
    out=parsed;return true;
}
struct Packet {std::uint32_t vehicle=0,pairing=0,sequence=0,uptime=0;Name label{};};
using Bytes=std::array<std::uint8_t,packetSize>;
inline void put(Bytes& b,unsigned pos,std::uint32_t n){for(unsigned i=0;i<4;++i)b[pos+i]=std::uint8_t(n>>(8*i));}
inline std::uint32_t get(const Bytes& b,unsigned pos){std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(b[pos+i])<<(8*i);return n;}
inline Bytes encode(const Packet& p) {
    Bytes b{};b[0]='R';b[1]='1';b[2]='5';b[3]='N';b[4]=1;
    while(b[5]<32&&p.label[b[5]])++b[5];
    put(b,8,p.vehicle);put(b,12,p.pairing);put(b,16,p.sequence);put(b,20,p.uptime);
    for(unsigned i=0;i<b[5];++i)b[24+i]=std::uint8_t(p.label[i]);
    put(b,60,gearlink::crc(b.data(),60));return b;
}
inline bool decode(const Bytes& b,Packet& out) {
    if(b[0]!='R'||b[1]!='1'||b[2]!='5'||b[3]!='N'||b[4]!=1||b[6]||b[7]||get(b,56)||get(b,60)!=gearlink::crc(b.data(),60))return false;
    Packet p;p.vehicle=get(b,8);p.pairing=get(b,12);p.sequence=get(b,16);p.uptime=get(b,20);
    if(!p.vehicle||!p.pairing||!name(b.data()+24,b[5],p.label))return false;
    for(unsigned i=b[5];i<32;++i)if(b[24+i])return false;
    out=p;return true;
}
class Receiver {
    Bytes bytes_{};unsigned size_=0;Packet latest_;bool seen_=false;std::uint64_t received_=0;
public:
    void reset(){*this=Receiver{};}
    void push(std::uint8_t byte,std::uint64_t now) {
        bytes_[size_++]=byte;if(size_<packetSize)return;
        Packet p;if(!decode(bytes_,p)){std::move(bytes_.begin()+1,bytes_.end(),bytes_.begin());--size_;return;}
        size_=0;
        if(seen_&&latest_.vehicle==p.vehicle){const auto step=p.sequence-latest_.sequence,age=p.uptime-latest_.uptime;if(!step||step>=0x80000000u||!age||age>=0x80000000u)return;}
        latest_=p;seen_=true;received_=now;
    }
    Name label(std::uint64_t now,std::uint32_t vehicle,std::uint32_t pairing)const {
        return seen_&&now>=received_&&now-received_<1000&&latest_.vehicle==vehicle&&latest_.pairing==pairing?latest_.label:Name{};
    }
};
}
