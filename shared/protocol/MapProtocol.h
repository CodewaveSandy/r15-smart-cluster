#pragma once
#include "GearPacket.h"
#include "shared/model/MapScene.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cluster::maplink {
constexpr unsigned maxMessage=240,maxPackage=65536;
struct Message {std::array<std::uint8_t,maxMessage> bytes{};unsigned size=0;};
using Capability=std::array<std::uint8_t,64>;
using Status=std::array<std::uint8_t,80>;
enum class Type : unsigned {TimeRequest=1,TimeConfirm,Begin,Data,Commit,Cancel,Location,Stop};
enum class State : unsigned {Unsupported,Empty,Receiving,Verifying,Active,Failed,Cancelled};
enum class Error : unsigned {None,Malformed,Unsupported,Session,Resource,Bounds,Offset,Hash,Timeout,Busy,Timing};
enum class LocationState : unsigned {NoAnchor,NoFix,Fresh,Stale,LowAccuracy,Outside,Stopped,Invalid};
inline std::uint64_t get(const std::uint8_t* b,unsigned p,unsigned n=4){std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(b[p+i])<<(i*8);return v;}
inline void put(std::uint8_t* b,unsigned p,std::uint64_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[p+i]=std::uint8_t(v>>(i*8));}
inline std::int32_t signed32(const std::uint8_t* b,unsigned p){const auto v=get(b,p);return v>INT32_MAX?std::int32_t(std::int64_t(v)-0x100000000LL):std::int32_t(v);}
inline int signed16(const std::uint8_t* b,unsigned p){const auto v=unsigned(get(b,p,2));return v>=32768?int(v)-65536:int(v);}
inline bool zero(const std::uint8_t* b,unsigned p,unsigned n){for(unsigned i=0;i<n;++i)if(b[p+i])return false;return true;}
inline bool magic(const std::uint8_t* b,char kind){return b[0]=='R'&&b[1]=='1'&&b[2]=='5'&&b[3]==kind;}
inline void prefix(std::uint8_t* b,char kind){b[0]='R';b[1]='1';b[2]='5';b[3]=std::uint8_t(kind);b[4]=1;}
inline void seal(std::uint8_t* b,unsigned n){put(b,n-4,gearlink::crc(b,n-4));}
inline bool intact(const std::uint8_t* b,unsigned n){return n>=8&&get(b,n-4)==gearlink::crc(b,n-4);}
inline bool forward(std::uint32_t n,std::uint32_t old){return n&&(!old||(n-old&&n-old<0x80000000u));}
inline Capability capability(std::uint32_t vehicle,std::uint32_t connection,std::uint32_t display,bool ready){
    Capability b{};prefix(b.data(),'K');b[5]=64;b[6]=ready?7:0;put(b.data(),8,vehicle);put(b.data(),12,connection);put(b.data(),16,display);
    put(b.data(),20,maxPackage);put(b.data(),24,2048,2);b[26]=8;b[27]=23;put(b.data(),28,maxMessage,2);put(b.data(),30,96,2);
    put(b.data(),32,4096,2);put(b.data(),34,500,2);put(b.data(),36,2000,2);put(b.data(),38,5000,2);seal(b.data(),b.size());return b;
}
inline bool validCapability(const std::uint8_t* b,unsigned n){return n==64&&magic(b,'K')&&b[4]==1&&b[5]==64&&b[6]<=7&&!b[7]&&zero(b,40,20)&&intact(b,n)&&get(b,20)>=64&&get(b,20)<=maxPackage&&get(b,24,2)>0&&get(b,24,2)<=2048&&b[26]==8&&b[27]==23&&get(b,28,2)>=93&&get(b,28,2)<=240&&get(b,30,2)==96&&get(b,32,2)>0&&get(b,32,2)<=4096&&get(b,34,2)==500&&get(b,36,2)==2000&&get(b,38,2)==5000;}
inline bool validStatus(const std::uint8_t* b,unsigned n){return n==80&&magic(b,'S')&&b[4]==1&&b[5]==80&&b[6]<=6&&b[7]<=10&&b[68]<=7&&b[69]<=1&&zero(b,70,2)&&get(b,28)<=get(b,32)&&get(b,32)<=maxPackage&&intact(b,n);}
inline bool validLocation(const std::uint8_t* p){
    if(p[34]>7||p[35]>4||!zero(p,36,4)||get(p,8,8)>INT64_MAX)return false;
    if((p[34]&4)&&(p[35]!=3||(p[34]&2)))return false;
    if(p[35]!=1)return !get(p,0,8)&&!get(p,16)&&!get(p,20)&&get(p,24)==UINT32_MAX&&get(p,28,2)==65535&&get(p,30,2)==65535&&get(p,32,2)==65535;
    const auto fix=get(p,0,8),sent=get(p,8,8);const auto lat=signed32(p,16),lon=signed32(p,20);
    return fix<=sent&&sent-fix<2000&&p[34]==1&&lat>=-850000000&&lat<=850000000&&lon>=-1800000000&&lon<=1800000000&&get(p,24)<=5000&&
        (get(p,28,2)<36000||get(p,28,2)==65535)&&(get(p,30,2)<=18000||get(p,30,2)==65535);
}
inline bool validCommand(const Message& m){
    const auto* b=m.bytes.data();if(m.size<36||m.size>240||!magic(b,'X')||b[4]!=1||b[5]<1||b[5]>8||b[6]!=32||b[7]||!zero(b,30,2)||!intact(b,m.size))return false;
    if(!get(b,8)||!get(b,12)||!get(b,16)||!get(b,24)||get(b,28,2)!=m.size-36)return false;
    const bool map=b[5]>=3&&b[5]<=6;if(bool(get(b,20))!=map)return false;
    const unsigned n=m.size-36;
    switch(Type(b[5])){
    case Type::TimeRequest:return n==8&&get(b,32,8)<=INT64_MAX;
    case Type::TimeConfirm:return n==20&&get(b,32)&&get(b,36,8)<=get(b,44,8)&&get(b,44,8)-get(b,36,8)<=500&&get(b,44,8)<=INT64_MAX;
    case Type::Begin:return n==36&&get(b,32)>=64&&get(b,32)<=maxPackage;
    case Type::Data:return n>=5&&n<=204&&get(b,32)<=maxPackage&&n-4<=maxPackage-get(b,32);
    case Type::Location:return n==40&&validLocation(b+32);
    default:return n==0;
    }
}
struct Region {
    std::int32_t lat=0,lon=0,south=0,west=0,north=0,east=0;
    maps::Point project(std::int32_t a,std::int32_t o)const{
        constexpr double k=6371008.8*3.141592653589793/180/10000000;
        return {std::int32_t(std::lround((double(o)-lon)*k*std::cos(lat*3.141592653589793/180/10000000))),std::int32_t(std::lround((double(a)-lat)*k))};
    }
    bool contains(std::int32_t a,std::int32_t o)const{return a>=south&&a<=north&&o>=west&&o<=east;}
};
// Incremental package reader; caller supplies reserved staging Scene storage.
class PackageReader {
public:
    bool begin(const std::uint8_t* b,unsigned n,maps::Scene& scene){
        data_=b;size_=n;scene_=&scene;index_=0;offset_=64;
        if(n<64||n>maxPackage||!magic(b,'B')||b[4]!=1||b[5]||get(b,6,2)!=64||get(b,34,2)||!zero(b,48,16)||get(b,36)!=n-64)return false;
        count_=unsigned(get(b,32,2));if(!count_||count_>scene.roads.size())return false;
        region={signed32(b,8),signed32(b,12),signed32(b,16),signed32(b,20),signed32(b,24),signed32(b,28)};
        if(region.south< -850000000||region.north>850000000||region.west< -1800000000||region.east>1800000000||region.south>=region.north||region.west>=region.east||!region.contains(region.lat,region.lon))return false;
        lower_=region.project(region.south,region.west);upper_=region.project(region.north,region.east);
        if(!maps::valid(lower_)||!maps::valid(upper_)||std::abs(lower_.east)>5000||std::abs(lower_.north)>5000||std::abs(upper_.east)>5000||std::abs(upper_.north)>5000)return false;
        scene.originLatE7=region.lat;scene.originLonE7=region.lon;scene.mercator=false;
        scene.roadCount=scene.routeCount=scene.overviewCount=0;scene.quality=Quality::Unavailable;return true;
    }
    // Return -1 invalid, 0 pending, 1 complete. At most budget road records.
    int step(unsigned budget=16){
        while(index_<count_&&budget--){
            if(offset_>size_||size_-offset_<4)return -1;
            const auto* b=data_+offset_;const unsigned points=b[1],name=b[2],length=4+name+points*4;
            if(b[0]>4||points<2||points>8||name>23||b[3]||length>size_-offset_)return -1;
            auto& r=scene_->roads[index_];r.count=points;r.kind=maps::RoadKind(b[0]);r.name.fill(0);bool named=false,distinct=false;
            for(unsigned i=0;i<name;++i){if(b[4+i]<32||b[4+i]>126)return -1;r.name[i]=char(b[4+i]);named|=b[4+i]!=32;}
            if(name&&!named)return -1;
            for(unsigned i=0;i<points;++i){const unsigned p=4+name+i*4;r.points[i]={signed16(b,p),signed16(b,p+2)};
                const auto v=r.points[i];if(!maps::valid(v)||v.east<lower_.east-1||v.east>upper_.east+1||v.north<lower_.north-1||v.north>upper_.north+1)return -1;
                distinct|=v.east!=r.points[0].east||v.north!=r.points[0].north;
            }
            if(!distinct)return -1;
            offset_+=length;++index_;
        }
        if(index_<count_)return 0;
        if(offset_!=size_)return -1;
        scene_->roadCount=count_;scene_->quality=Quality::Available;return 1;
    }
    Region region;
private:
    const std::uint8_t* data_=nullptr;maps::Scene* scene_=nullptr;unsigned size_=0,count_=0,index_=0,offset_=0;maps::Point lower_,upper_;
};
}
