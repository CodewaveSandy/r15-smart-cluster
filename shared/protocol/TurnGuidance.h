#pragma once
#include "Binary.h"
#include "shared/model/TurnGuidanceState.h"
#include <algorithm>
#include <array>
#include <cstring>
namespace cluster::guidance {
using namespace binary;
constexpr unsigned maxCommand=224,minimumMtu=227;
constexpr std::uint64_t maxElapsed=0x7fffffffffffffffull;
constexpr std::uint32_t transportTtl=5000,dimAge=30000,hideAge=120000,anchorTtl=30000;
enum class Command:std::uint8_t {Open=1,TimeRequest,TimeConfirm,Snapshot,Presence,Clear};
struct Message {std::array<std::uint8_t,maxCommand> bytes{};unsigned size=0;};
using Capability=std::array<std::uint8_t,80>;
using Status=std::array<std::uint8_t,112>;
inline bool magic(const std::uint8_t* b,char type){return b[0]=='R'&&b[1]=='1'&&b[2]=='5'&&b[3]==type;}
inline void header(std::uint8_t* b,char type,unsigned n){b[0]='R';b[1]='1';b[2]='5';b[3]=type;b[4]=1;b[6]=std::uint8_t(n);}
inline bool fieldText(const std::uint8_t* b,unsigned used,unsigned capacity){
    if(used>capacity||!zero(b,used,capacity-used))return false;
    bool visible=false;for(unsigned i=0;i<used;++i){if(b[i]<32||b[i]>126)return false;visible|=b[i]!=' ';}return !used||visible;
}
inline bool valueFlag(std::uint16_t f,unsigned bit,std::uint32_t v,std::uint32_t max){return (f&bit)?v<=max:v==unknown;}
inline bool decodeSnapshot(const Message& m,Snapshot& out){
    if(m.size!=224)return false;
    const auto* b=m.bytes.data()+40;Snapshot s;s.source=Source(b[0]);s.maneuver=Maneuver(b[1]);s.flags=std::uint16_t(get(b,2,2));
    if(b[0]>6||(b[1]>8&&b[1]!=255)||(s.flags&~1023u))return false;
    s.next=std::uint32_t(get(b,4));s.remaining=std::uint32_t(get(b,8));s.seconds=std::uint32_t(get(b,12));s.exit=b[16];s.changed=get(b,20,8);s.dispatch=get(b,28,8);
    if(!s.dispatch||s.dispatch>maxElapsed||!valueFlag(s.flags,NextDistance,s.next,1000000)||!valueFlag(s.flags,RemainingDistance,s.remaining,10000000)||!valueFlag(s.flags,RemainingTime,s.seconds,604800))return false;
    if(bool(s.flags&Direction)!=(s.maneuver!=Maneuver::Unknown)||bool(s.flags&Exit)!=(s.exit!=0)||s.exit>32||(s.exit&&s.maneuver!=Maneuver::Roundabout))return false;
    const unsigned at[]={36,132,164},length[]={b[17],b[18],b[19]},capacity[]={96,32,16},bits[]={Instruction,Road,Summary};
    for(unsigned i=0;i<3;++i)if(bool(s.flags&bits[i])!=(length[i]!=0)||!fieldText(b+at[i],length[i],capacity[i]))return false;
    if(s.source==Source::Guiding&&!(s.flags&Instruction))return false;
    if(s.source!=Source::Guiding){
        if(s.flags&(NextDistance|RemainingDistance|RemainingTime|Summary|Exit))return false;
        if((s.flags&Direction)&&(s.source!=Source::Arrived||s.maneuver!=Maneuver::Arrive))return false;
    }
    const auto age=get(m.bytes.data(),32);
    if(s.flags&AgeKnown){if(s.changed==unknownTime||s.dispatch<s.changed||s.dispatch-s.changed>=unknown||age!=s.dispatch-s.changed)return false;}
    else if(s.changed!=unknownTime||age!=unknown)return false;
    std::copy_n(b+36,length[0],s.instruction.data());std::copy_n(b+132,length[1],s.road.data());std::copy_n(b+164,length[2],s.summary.data());out=s;return true;
}
inline bool sameContent(const Snapshot& a,const Snapshot& b){
    constexpr unsigned mask=Instruction|Road|Direction|NextDistance|Exit|Truncated;
    return a.source==b.source&&a.maneuver==b.maneuver&&(a.flags&mask)==(b.flags&mask)&&a.next==b.next&&a.exit==b.exit&&a.instruction==b.instruction&&a.road==b.road;
}
inline bool validCommand(const Message& m){
    const auto* b=m.bytes.data();if(m.size<44||m.size>224||!magic(b,'U')||b[4]!=1||b[5]<1||b[5]>6||b[6]!=40||b[7]||get(b,38,2)||get(b,36,2)+44!=m.size||!intact(b,m.size))return false;
    for(unsigned at:{8u,12u,16u,20u,24u})if(!get(b,at))return false;
    const auto c=Command(b[5]);const auto* body=b+40;const auto n=get(b,36,2);
    if(c==Command::Snapshot){Snapshot s;return get(b,28)&&decodeSnapshot(m,s);}
    if(get(b,32)!=unknown||(c!=Command::Presence&&get(b,28)))return false;
    switch(c){
    case Command::Open:return n==4&&body[0]==1&&zero(body,1,3);
    case Command::TimeRequest:return n==8&&get(body,0,8)&&get(body,0,8)<=maxElapsed;
    case Command::TimeConfirm:return n==32&&get(body,0)&&zero(body,4,4)&&get(body,8,8)&&get(body,16,8)<=maxElapsed&&get(body,16,8)>=get(body,8,8)&&get(body,16,8)-get(body,8,8)<=500;
    case Command::Presence:return n==4&&body[0]<=15&&zero(body,1,3)&&(!(body[0]&2)||(body[0]&1))&&(!(body[0]&4)||(body[0]&2));
    case Command::Clear:return n==4&&body[0]>=1&&body[0]<=6&&zero(body,1,3);
    default:return false;
    }
}
inline bool sameCore(const Snapshot& a,const Snapshot& b){
    constexpr unsigned mask=Instruction|Road|Direction|NextDistance|Exit|Truncated|AgeKnown;
    return a.source==b.source&&a.maneuver==b.maneuver&&(a.flags&mask)==(b.flags&mask)&&a.next==b.next&&a.exit==b.exit&&a.changed==b.changed&&a.instruction==b.instruction&&a.road==b.road;
}
inline Capability capabilities(std::uint32_t boot,std::uint32_t connection,std::uint32_t display,bool ready,std::uint64_t now,unsigned mtu=0){
    Capability b{};header(b.data(),'F',b.size());b[7]=ready?1:0;put(b.data(),8,boot);put(b.data(),12,connection);put(b.data(),16,display);
    put(b.data(),20,224,2);put(b.data(),22,227,2);put(b.data(),24,mtu,2);b[26]=1;put(b.data(),28,511,2);b[30]=96;b[31]=32;b[32]=16;b[33]=2;put(b.data(),34,2000,2);
    put(b.data(),36,5000);put(b.data(),40,30000);put(b.data(),44,120000);put(b.data(),48,30000);put(b.data(),52,500,2);put(b.data(),54,25000,2);put(b.data(),56,now,8);seal(b.data(),b.size());return b;
}
inline bool validCapability(const Message& m){
    if(m.size!=80||!magic(m.bytes.data(),'F')||m.bytes[4]!=1||m.bytes[5]||m.bytes[6]!=80||m.bytes[7]>1||!intact(m.bytes.data(),80)||!zero(m.bytes.data(),64,12))return false;
    const auto& b=m.bytes;const auto expected=capabilities(std::uint32_t(get(b.data(),8)),std::uint32_t(get(b.data(),12)),std::uint32_t(get(b.data(),16)),b[7],get(b.data(),56,8),unsigned(get(b.data(),24,2)));
    return get(b.data(),16)&&(!(b[7]&1)||(get(b.data(),8)&&get(b.data(),12)))&&(get(b.data(),24,2)==0||(get(b.data(),24,2)>=23&&get(b.data(),24,2)<=517))&&std::equal(expected.begin(),expected.end(),b.begin());
}
inline bool validStatus(const Message& m){
    const auto* b=m.bytes.data();
    if(m.size!=112||!magic(b,'W')||b[4]!=1||b[5]||b[6]!=112||b[32]>6||b[33]>8||b[34]>8||b[35]>6||!zero(b,60,4)||!get(b,16)||!intact(b,112))return false;
    if(bool(b[7]&32)!=(get(b,36)!=unknown))return false;
    if((b[7]&1)&&(!get(b,8)||!get(b,12)||!get(b,20)))return false;
    if((b[7]&(2|16))&&!(b[7]&1))return false;
    if((b[7]&128)&&!(b[7]&4))return false;
    if((b[7]&8)&&((b[7]&51)!=51||b[33]!=std::uint8_t(Quality::Fresh)||get(b,36)>=dimAge||(b[32]!=1&&b[32]!=3)))return false;
    for(unsigned at:{40u,48u,64u,72u,88u})if(get(b,at,8)>maxElapsed)return false;
    const auto changed=get(b,80,8);if(changed!=unknownTime&&changed>maxElapsed)return false;
    if(get(b,56)&&(!get(b,64,8)||!get(b,72,8)))return false;
    return true;
}
template<std::size_t N> inline Message message(const std::array<std::uint8_t,N>& b){static_assert(N<=224);Message m;m.size=N;std::copy(b.begin(),b.end(),m.bytes.begin());return m;}
inline Message command(Command type,std::uint32_t boot,std::uint32_t connection,std::uint32_t display,std::uint32_t stream,std::uint32_t sequence,unsigned body){
    Message m;m.size=44+body;header(m.bytes.data(),'U',40);m.bytes[5]=std::uint8_t(type);put(m.bytes.data(),8,boot);put(m.bytes.data(),12,connection);put(m.bytes.data(),16,display);put(m.bytes.data(),20,stream);put(m.bytes.data(),24,sequence);put(m.bytes.data(),32,unknown);put(m.bytes.data(),36,body,2);return m;
}
}
