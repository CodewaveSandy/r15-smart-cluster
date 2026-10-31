#pragma once
#include "shared/protocol/PairingIdentity.h"
namespace cluster::phonemeta {
// Portable phone metadata only; no presentation, radio or clock-service dependencies.
struct Data {pairidentity::Name label{};std::uint32_t epoch=0;int offset=0,battery=-1;};
using Payload=std::array<std::uint8_t,44>;
inline bool decodePayload(const Payload& b,Data& out) {
    if(b[0]!=1||b[3]||b[10]||b[11]||(b[2]>100&&b[2]!=255))return false;
    Data p;if(b[1]&&!pairidentity::name(b.data()+12,b[1],p.label))return false;
    if(b[1]>32)return false;
    for(unsigned i=b[1];i<32;++i)if(b[12+i])return false;
    for(unsigned i=0;i<4;++i)p.epoch|=std::uint32_t(b[4+i])<<(8*i);
    const unsigned raw=unsigned(b[8])|(unsigned(b[9])<<8);p.offset=raw>=32768?int(raw)-65536:int(raw);
    if((p.epoch&&(p.epoch<1577836800u||p.epoch>4102444799u))||p.offset< -720||p.offset>840||(!p.epoch&&p.offset))return false;
    p.battery=b[2]==255?-1:b[2];out=p;return true;
}
inline Payload encodePayload(const Data& p) {
    Payload b{};b[0]=1;b[2]=p.battery<0?255:std::uint8_t(p.battery);
    while(b[1]<32&&p.label[b[1]]){b[12+b[1]]=std::uint8_t(p.label[b[1]]);++b[1];}
    for(unsigned i=0;i<4;++i)b[4+i]=std::uint8_t(p.epoch>>(8*i));
    b[8]=std::uint8_t(p.offset);b[9]=std::uint8_t(unsigned(p.offset)>>8);return b;
}
struct Packet {std::uint32_t vehicle=0,connection=0,sequence=0,uptime=0;Data data;};
using Bytes=std::array<std::uint8_t,80>;
inline void put(Bytes& b,unsigned pos,std::uint32_t n){for(unsigned i=0;i<4;++i)b[pos+i]=std::uint8_t(n>>(8*i));}
inline std::uint32_t get(const Bytes& b,unsigned pos){std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(b[pos+i])<<(8*i);return n;}
inline Bytes encode(const Packet& p) {
    Bytes b{};b[0]='R';b[1]='1';b[2]='5';b[3]='M';b[4]=1;
    put(b,8,p.vehicle);put(b,12,p.connection);put(b,16,p.sequence);put(b,20,p.uptime);
    const auto payload=encodePayload(p.data);std::copy(payload.begin(),payload.end(),b.begin()+24);
    put(b,76,gearlink::crc(b.data(),76));return b;
}
class Receiver {
    Bytes bytes_{};unsigned size_=0;Packet latest_;bool seen_=false;std::uint64_t received_=0;
public:
    void reset(){*this=Receiver{};}
    void push(std::uint8_t byte,std::uint64_t now) {
        bytes_[size_++]=byte;if(size_<bytes_.size())return;
        const auto& b=bytes_;Payload payload{};std::copy(b.begin()+24,b.begin()+68,payload.begin());Data data;
        bool ok=b[0]=='R'&&b[1]=='1'&&b[2]=='5'&&b[3]=='M'&&b[4]==1&&!b[5]&&!b[6]&&!b[7]&&get(b,8)&&get(b,12)&&!get(b,68)&&!get(b,72)&&get(b,76)==gearlink::crc(b.data(),76)&&decodePayload(payload,data);
        if(!ok){std::move(bytes_.begin()+1,bytes_.end(),bytes_.begin());--size_;return;}
        size_=0;Packet p{get(b,8),get(b,12),get(b,16),get(b,20),data};
        if(seen_&&latest_.vehicle==p.vehicle){const auto step=p.sequence-latest_.sequence,age=p.uptime-latest_.uptime;if(!step||step>=0x80000000u||!age||age>=0x80000000u)return;}
        latest_=p;seen_=true;received_=now;
    }
    const Packet* current(std::uint64_t now,std::uint32_t vehicle,std::uint32_t connection)const {
        return seen_&&now>=received_&&now-received_<1000&&latest_.vehicle==vehicle&&latest_.connection==connection?&latest_:nullptr;
    }
};
}
