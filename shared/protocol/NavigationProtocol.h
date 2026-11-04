#pragma once
#include "MapProtocol.h"
#include "shared/model/MapProjection.h"
namespace cluster::navlink {
using maplink::get;using maplink::put;using maplink::zero;using maplink::magic;using maplink::seal;using maplink::intact;using maplink::signed32;
using Message=maplink::Message;using Digest=std::array<std::uint8_t,32>;using Capability=std::array<std::uint8_t,64>;
inline constexpr unsigned maxObject=32768,cacheSlots=12,window=4,maxCellRoads=512,maxRoutePoints=512,header=40;
enum class Op:unsigned {Open=1,TimeRequest,TimeConfirm,Begin,Data,Commit,Cancel,Location,Activate,Progress,Stop,Have,End};
enum class Reply:unsigned {Status=64,Interest=65,RouteNeed=66};
enum class Kind:unsigned {Cell=1,Overview=2,RouteWindow=3};
enum class Stage:unsigned {Closed,Idle,Receiving,Verifying,Ready,Stopped,Have};
enum class Error:unsigned {None,Sequence,Session,Busy,Bounds,Hash,Offset,Resource,Timeout,NotReady,Timing,Conflict,Unsupported};
enum class Fix:unsigned {None,Precise,Approximate,Stale,Invalid,Stopped,NoAnchor};
enum class Coverage:unsigned {Missing,Partial,Ready};
inline unsigned bodySize(const Message& m){return m.size>=44?m.size-44:0;}
inline const std::uint8_t* body(const Message& m){return m.bytes.data()+header;}
inline bool envelope(const Message& m,bool reply=false){
    const auto* b=m.bytes.data();return m.size>=44&&m.size<=240&&magic(b,reply?'Y':'V')&&b[4]==2&&b[6]==40&&!b[7]&&!get(b,38,2)&&get(b,36,2)==m.size-44&&intact(b,m.size);
}
inline Message message(unsigned op,bool reply,std::uint32_t vehicle,std::uint32_t connection,std::uint32_t display,std::uint32_t session,std::uint32_t seq,std::uint32_t object=0,std::uint32_t offset=0,unsigned n=0){
    Message m;if(n>196)return m;m.size=44+n;auto* b=m.bytes.data();maplink::prefix(b,reply?'Y':'V');b[4]=2;b[5]=std::uint8_t(op);b[6]=40;
    put(b,8,vehicle);put(b,12,connection);put(b,16,display);put(b,20,session);put(b,24,seq);put(b,28,object);put(b,32,offset);put(b,36,n,2);return m;
}
inline bool command(const Message& m){
    if(!envelope(m))return false;
    const auto* b=m.bytes.data();const unsigned n=bodySize(m);const auto* p=body(m);
    if(!get(b,8)||!get(b,12)||!get(b,16)||!get(b,20)||!get(b,24)||b[5]<1||b[5]>13)return false;
    const auto op=Op(b[5]);const bool object=op==Op::Begin||op==Op::Data||op==Op::Commit||op==Op::Cancel||op==Op::Have;
    if(bool(get(b,28))!=object||(op!=Op::Data&&get(b,32)))return false;
    switch(op){
    case Op::Open:return n==40&&get(b,24)==1&&get(p,32,2)==1&&get(p,34,2)>=1&&get(p,34,2)<=2&&p[36]>=1&&p[36]<=4&&zero(p,37,3)&&!zero(p,0,32);
    case Op::TimeRequest:return n==8&&get(p,0,8)<=INT64_MAX;
    case Op::TimeConfirm:return n==20&&get(p,0)&&get(p,4,8)<=get(p,12,8)&&get(p,12,8)-get(p,4,8)<=500&&get(p,12,8)<=INT64_MAX;
    case Op::Begin:return n==40&&get(p,0)>=64&&get(p,0)<=maxObject&&p[36]>=1&&p[36]<=3&&!p[37]&&zero(p,38,2);
    case Op::Data:return n>=1&&get(b,32)<maxObject&&n<=maxObject-get(b,32);
    case Op::Commit:case Op::Cancel:case Op::Stop:case Op::End:return !n;
    case Op::Have:return n==40&&get(p,32)>=64&&get(p,32)<=maxObject&&zero(p,36,4);
    case Op::Location:{
        if(n!=40||!zero(p,36,4)||p[34]>3||p[35]>3||get(p,8,8)>INT64_MAX)return false;
        if(p[35]!=1)return !get(p,0,8)&&!get(p,16)&&!get(p,20)&&get(p,24)==UINT32_MAX&&get(p,28,2)==65535&&get(p,30,2)==65535&&get(p,32,2)==65535;
        return p[34]>=1&&p[34]<=2&&get(p,0,8)<=get(p,8,8)&&get(p,8,8)-get(p,0,8)<2000&&
            signed32(p,16)>=-850511287&&signed32(p,16)<=850511287&&signed32(p,20)>=-1800000000&&signed32(p,20)<=1800000000&&get(p,24)<=100000000&&
            (get(p,28,2)<36000||get(p,28,2)==65535)&&(get(p,30,2)<=18000||get(p,30,2)==65535);
    }
    case Op::Activate:return n==24&&get(p,0,8)&&get(p,8)&&get(p,12)&&get(p,16)&&get(p,20);
    case Op::Progress:return n==40&&get(p,0,8)&&get(p,8)&&get(p,12,8)<=INT64_MAX&&get(p,20)<=100000000&&get(p,24)<=31536000&&get(p,28)<=100000000&&p[32]<=7&&p[33]<=1&&zero(p,34,6);
    }return false;
}
inline bool response(const Message& m){
    if(!envelope(m,true))return false;
    const auto* p=body(m);const unsigned op=m.bytes[5];
    return op==64?(bodySize(m)==40&&p[0]<=6&&p[1]<=12&&p[2]>=1&&p[2]<=4&&p[3]<=4&&p[12]<=6&&p[13]<=2&&p[14]<=1&&p[15]<=1):
        op==65?(bodySize(m)==44&&p[8]<=22&&p[9]<=4&&p[10]<=15&&p[11]<=1):op==66&&bodySize(m)==24&&get(p,0,8)&&get(p,8)&&get(p,16)<=get(p,20);
}
inline Capability capability(std::uint32_t vehicle,std::uint32_t connection,std::uint32_t display,bool ready,unsigned arena){
    Capability b{};maplink::prefix(b.data(),'J');b[4]=2;b[5]=64;b[6]=ready?15:0;b[7]=4;
    put(b.data(),8,vehicle);put(b.data(),12,connection);put(b.data(),16,display);put(b.data(),20,arena);put(b.data(),24,maxObject);
    b[28]=cacheSlots;b[29]=4;put(b.data(),30,maxCellRoads,2);put(b.data(),32,2048,2);put(b.data(),34,maxRoutePoints,2);put(b.data(),36,240,2);put(b.data(),38,96,2);
    put(b.data(),40,4096,2);put(b.data(),42,2000,2);put(b.data(),44,30000,2);put(b.data(),46,10000,2);put(b.data(),48,1);put(b.data(),52,3);b[56]=6;b[57]=2;seal(b.data(),64);return b;
}
inline bool validCapability(const Message& m){const auto* b=m.bytes.data();return m.size==64&&magic(b,'J')&&b[4]==2&&b[5]==64&&(b[6]==0||b[6]==15)&&b[7]==4&&get(b,20)<=1048576&&get(b,24)==maxObject&&b[28]==cacheSlots&&b[29]==4&&get(b,30,2)==512&&get(b,32,2)==2048&&get(b,34,2)==512&&get(b,36,2)==240&&get(b,38,2)==96&&get(b,40,2)==4096&&get(b,42,2)==2000&&get(b,44,2)==30000&&get(b,46,2)==10000&&get(b,48)==1&&get(b,52)==3&&b[56]==6&&b[57]==2&&zero(b,58,2)&&intact(b,64);}
struct Object {
    Kind kind=Kind::Cell;std::uint32_t source=0,revision=0,x=0,y=0,distance=0,seconds=0,start=0,end=0;
    std::uint64_t route=0;unsigned z=0,count=0,attribution=1;bool partial=false;
};
inline bool objectHeader(const std::uint8_t* b,unsigned size,Object& o){
    if(size<64||size>maxObject||!magic(b,'O')||b[4]!=2||b[5]<1||b[5]>3||b[6]!=64||b[7]>1||b[29]||get(b,60)||get(b,12,2)!=1||get(b,14,2)<1||get(b,14,2)>2)return false;
    o.attribution=unsigned(get(b,14,2));o.partial=b[7]==1;
    o.kind=Kind(b[5]);o.source=std::uint32_t(get(b,8));o.route=get(b,16,8);o.revision=std::uint32_t(get(b,24));o.z=b[28];o.count=unsigned(get(b,30,2));o.x=std::uint32_t(get(b,32));o.y=std::uint32_t(get(b,36));
    o.start=std::uint32_t(get(b,40));o.end=std::uint32_t(get(b,44));o.distance=std::uint32_t(get(b,48));o.seconds=std::uint32_t(get(b,52));
    if(o.kind==Kind::Cell)return !o.route&&!o.revision&&!o.start&&!o.end&&!o.distance&&!o.seconds&&o.z<=22&&o.x<(1u<<o.z)&&o.y<(1u<<o.z)&&o.count<=maxCellRoads;
    return !o.partial&&o.route&&o.revision&&!o.z&&!o.x&&!o.y&&o.count>=2&&o.count<=maxRoutePoints&&size==64+8*o.count&&o.start<=o.end&&o.end<=o.distance&&o.distance&&o.distance<=100000000&&o.seconds<=31536000&&
        (o.kind==Kind::Overview?o.start==0&&o.end==o.distance:o.start<o.end);
}
}
