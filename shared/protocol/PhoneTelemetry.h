#pragma once
#include "shared/protocol/GearPacket.h"
#include <limits>

namespace cluster::phonetelemetry {
constexpr std::uint8_t version=1;
constexpr unsigned packetSize=64,healthSize=80,capabilitySize=20,minimumMtu=67,healthMtu=83,intervalMs=200,staleMs=1000;
constexpr std::uint16_t unknownAge=0xffff;
using Bytes=std::array<std::uint8_t,packetSize>;
using Capabilities=std::array<std::uint8_t,capabilitySize>;
template<std::size_t N> inline void put(std::array<std::uint8_t,N>& b,unsigned pos,std::uint32_t n) {
    for(unsigned i=0;i<4;++i)b[pos+i]=std::uint8_t(n>>(8*i));
}
template<std::size_t N> inline std::uint32_t get(const std::array<std::uint8_t,N>& b,unsigned pos) {
    std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(b[pos+i])<<(8*i);return n;
}
inline std::uint16_t age(std::uint64_t elapsed) {return std::uint16_t(elapsed<unknownAge?elapsed:unknownAge-1);}
inline Capabilities capabilities(std::uint32_t boot) {
    Capabilities b{};b[0]='R';b[1]='1';b[2]='5';b[3]='C';b[4]=version;b[6]=1;
    put(b,8,boot);put(b,12,7);b[16]=packetSize;b[18]=intervalMs;return b;
}
struct Snapshot {
    std::uint32_t boot=0,connection=0,sequence=0,uptime=0;
    std::uint16_t sourceAge=unknownAge,metadataAgeSeconds=unknownAge;
    bool displayFresh=false,metadataFresh=false;
    gearlink::Packet inputs;
};
inline Bytes encode(const Snapshot& p) {
    Bytes b{};b[0]='R';b[1]='1';b[2]='5';b[3]='T';b[4]=version;b[5]=packetSize;
    const bool fresh=p.sourceAge!=unknownAge&&p.sourceAge<staleMs;
    b[6]=std::uint8_t(p.displayFresh|(p.metadataFresh<<1)|(fresh<<2));
    put(b,8,p.boot);put(b,12,p.connection);put(b,16,p.sequence);put(b,20,p.uptime);
    b[24]=std::uint8_t(p.sourceAge);b[25]=std::uint8_t(p.sourceAge>>8);
    b[26]=std::uint8_t(p.metadataAgeSeconds);b[27]=std::uint8_t(p.metadataAgeSeconds>>8);
    const auto quality=[&](Quality q){return std::uint8_t(p.sourceAge==unknownAge?Quality::Unavailable:fresh?q:Quality::Stale);};
    b[28]=quality(p.inputs.gear.quality);b[29]=b[28]==unsigned(Quality::Available)?std::uint8_t(p.inputs.gear.value):255;
    const Signal<bool> lamps[]={p.inputs.left,p.inputs.right,p.inputs.highBeam};
    for(unsigned i=0;i<3;++i){b[30+2*i]=quality(lamps[i].quality);b[31+2*i]=b[30+2*i]==unsigned(Quality::Available)&&lamps[i].value;}
    // No real speed/RPM/fuel/voltage source exists. Reserved value slots stay zero.
    for(unsigned i=36;i<40;++i)b[i]=unsigned(Quality::Unsupported);
    if(p.sourceAge!=unknownAge){put(b,48,p.inputs.tripPresses);put(b,52,p.inputs.infoPresses);put(b,56,p.inputs.sequence);}
    put(b,60,gearlink::crc(b.data(),60));return b;
}
// Strict portable validation for normative captures and future C++ consumers.
inline bool decode(const Bytes& b,Snapshot& out) {
    if(b[0]!='R'||b[1]!='1'||b[2]!='5'||b[3]!='T'||b[4]!=version||b[5]!=packetSize||b[6]>7||b[7]||
       !get(b,8)||!get(b,12)||get(b,40)||get(b,44)||get(b,60)!=gearlink::crc(b.data(),60))return false;
    Snapshot p;p.boot=get(b,8);p.connection=get(b,12);p.sequence=get(b,16);p.uptime=get(b,20);
    p.sourceAge=std::uint16_t(unsigned(b[24])|(unsigned(b[25])<<8));
    p.metadataAgeSeconds=std::uint16_t(unsigned(b[26])|(unsigned(b[27])<<8));p.displayFresh=b[6]&1;p.metadataFresh=b[6]&2;
    if(bool(b[6]&4)!=(p.sourceAge!=unknownAge&&p.sourceAge<staleMs)||
       (p.metadataFresh&&(p.metadataAgeSeconds==unknownAge||p.metadataAgeSeconds>=90)))return false;
    if(b[28]>unsigned(Quality::Stale)||(b[28]==unsigned(Quality::Available)?b[29]>6:b[29]!=255))return false;
    if(p.sourceAge==unknownAge?(b[28]!=unsigned(Quality::Unavailable)||get(b,48)||get(b,52)||get(b,56)):
       p.sourceAge>=staleMs&&b[28]!=unsigned(Quality::Stale))return false;
    p.inputs.session=p.boot;p.inputs.sequence=get(b,56);p.inputs.tripPresses=get(b,48);p.inputs.infoPresses=get(b,52);
    p.inputs.gear={b[29]==255?0:int(b[29]),Quality(b[28])};
    Signal<bool>* lamps[]={&p.inputs.left,&p.inputs.right,&p.inputs.highBeam};
    for(unsigned i=0;i<3;++i){const auto q=b[30+2*i],value=b[31+2*i];
        if(q>unsigned(Quality::Stale)||value>1||(q!=unsigned(Quality::Available)&&value)||
           (p.sourceAge==unknownAge?q!=unsigned(Quality::Unavailable):p.sourceAge>=staleMs&&q!=unsigned(Quality::Stale)))return false;
        *lamps[i]={bool(value),Quality(q)};
    }
    for(unsigned i=36;i<40;++i)if(b[i]!=unsigned(Quality::Unsupported))return false;
    out=p;return true;
}
struct Diagnostics {
    std::uint32_t accepted=0,rejected=0,invalid=0,rateLimited=0,expired=0;
    std::uint32_t connections=0,disconnects=0,notifyQueued=0,notifyCompleted=0,notifyErrors=0;
    std::uint32_t maxMetadataGapMs=0,maxHostGapMs=0,maxSourceGapMs=0;
};
using Health=std::array<std::uint8_t,healthSize>;
inline Health diagnostics(const Diagnostics& p,std::uint32_t boot,std::uint32_t connection,std::uint32_t uptime) {
    Health b{};b[0]='R';b[1]='1';b[2]='5';b[3]='H';b[4]=version;b[5]=healthSize;
    put(b,8,boot);put(b,12,connection);put(b,16,uptime);
    const std::uint32_t counters[]={p.accepted,p.rejected,p.invalid,p.rateLimited,p.expired,p.connections,p.disconnects,
        p.notifyQueued,p.notifyCompleted,p.notifyErrors};
    for(unsigned i=0;i<10;++i)put(b,20+4*i,counters[i]);
    put(b,60,p.maxMetadataGapMs);put(b,64,p.maxHostGapMs);put(b,68,p.maxSourceGapMs);
    put(b,76,gearlink::crc(b.data(),76));return b;
}
inline void increment(std::uint32_t& n) {if(n!=std::numeric_limits<std::uint32_t>::max())++n;}
}
