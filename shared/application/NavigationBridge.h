#pragma once
#include "shared/protocol/NavigationProtocol.h"
namespace cluster::navlink {
// Authenticated vehicle ingress invokes this under its existing lock. Contains
// fragments/credits only: no city, routing, GPS calculation or display objects.
class NavigationBridge {
public:
    void initialize(std::uint32_t vehicle){vehicle_=vehicle;}
    void invalidate(){for(auto& e:entries_)e={};latestPending_[0]=latestPending_[1]=false;capAt_=statusAt_=0;nav_=lastAck_=0;dirty_=true;interestDirty_=routeNeedDirty_=false;stopped_=false;}
    std::uint32_t session()const{return nav_;}
    unsigned maxAckMs()const{return maxAckMs_;}
    unsigned highWater()const{return highWater_;}
    void downstream(const Message& m,std::uint64_t now,std::uint32_t connection){
        if(validCapability(m)&&get(m.bytes.data(),8)==vehicle_){
            if(get(cap_.data(),16)!=get(m.bytes.data(),16))invalidate();
            std::copy_n(m.bytes.data(),64,cap_.begin());capAt_=now;dirty_=true;return;
        }
        if(!response(m)||get(m.bytes.data(),8)!=vehicle_||get(m.bytes.data(),12)!=connection||get(m.bytes.data(),16)!=get(cap_.data(),16)||get(m.bytes.data(),20)!=nav_)return;
        if(m.bytes[5]==65){if(interest_.size&&get(interest_.bytes.data(),20)==nav_&&get(m.bytes.data(),24)!=get(interest_.bytes.data(),24)&&!maplink::forward(std::uint32_t(get(m.bytes.data(),24)),std::uint32_t(get(interest_.bytes.data(),24))))return;interest_=m;interestDirty_=true;return;}
        if(m.bytes[5]==66){routeNeed_=m;routeNeedDirty_=true;return;}
        const auto seq=std::uint32_t(get(m.bytes.data(),24));
        if(lastAck_&&seq!=lastAck_&&!maplink::forward(seq,lastAck_))return;
        bool known=seq==lastAck_;for(const auto& e:entries_)if(e.nav==nav_&&e.seq==seq&&e.sent)known=true;
        if(!known)return;
        if(statusAt_&&get(body(m),16,8)<get(body(status_),16,8))return;
        status_=m;statusAt_=now;lastAck_=seq;
        for(auto& e:entries_)if(e.busy&&(e.seq==seq||maplink::forward(seq,e.seq))){
            if(now>=e.acceptedAt)maxAckMs_=std::max(maxAckMs_,unsigned(std::min<std::uint64_t>(UINT32_MAX,now-e.acceptedAt)));
            e.busy=false;e.acked=true;
        }
        dirty_=true;
    }
    Capability capability(std::uint64_t now,std::uint32_t connection,bool displayFresh)const{
        auto b=cap_;if(!capAt_||now<capAt_||now-capAt_>1500||!displayFresh)b=navlink::capability(vehicle_,connection,0,false,0);
        put(b.data(),12,connection);seal(b.data(),64);return b;
    }
    unsigned credits()const{unsigned used=0;for(const auto& e:entries_)used+=e.busy?1:0;return used>=window_?0:window_-used;}
    Message status(std::uint64_t now,std::uint32_t connection,bool displayFresh)const{
        auto m=status_;
        if(!statusAt_||now<statusAt_||now-statusAt_>=2000||!displayFresh){
            m=message(64,true,vehicle_,connection,std::uint32_t(get(cap_.data(),16)),nav_,lastAck_,0,0,40);auto* p=m.bytes.data()+40;p[0]=0;p[2]=std::uint8_t(window_);p[12]=std::uint8_t(Fix::NoAnchor);
        }else m.bytes[43]=std::uint8_t(std::min<unsigned>(m.bytes[43],credits()));
        seal(m.bytes.data(),m.size);return m;
    }
    bool admit(const Message& m,std::uint64_t now,std::uint32_t connection,bool displayFresh){
        if(!navlink::command(m))return false;
        const auto cap=capability(now,connection,displayFresh);const auto* b=m.bytes.data();
        if(cap[6]!=15||get(b,8)!=vehicle_||get(b,12)!=connection||get(b,16)!=get(cap.data(),16))return false;
        const auto op=Op(b[5]);const auto seq=std::uint32_t(get(b,24)),nav=std::uint32_t(get(b,20));
        if(op==Op::Location||op==Op::Progress){
            const unsigned i=op==Op::Location?0:1;if(stopped_||nav!=nav_||!nav_||(!statusAt_)||(latestAt_[i]&&now-latestAt_[i]<500))return false;
            latest_[i]=m;latestPending_[i]=true;latestAt_[i]=now;return true;
        }
        for(auto& e:entries_)if(e.nav==nav&&e.seq==seq&&e.message.size){
            if(e.message.size!=m.size||!std::equal(m.bytes.begin(),m.bytes.begin()+m.size,e.message.bytes.begin()))return false;
            if(e.busy)e.sent=false;else if(!e.acked)return false;dirty_=true;return true;
        }
        if(op==Op::Open){
            if(nav_&&!maplink::forward(nav,nav_))return false;
            for(auto& e:entries_)e={};
            latestPending_[0]=latestPending_[1]=false;latestAt_[0]=latestAt_[1]=0;interestDirty_=routeNeedDirty_=false;interest_.size=0;stopped_=false;
            nav_=nav;lastAck_=0;statusAt_=0;window_=body(m)[36];
        }else{
            if(nav!=nav_||!nav_)return false;
            if(op==Op::Stop||op==Op::End||op==Op::Cancel){for(auto& e:entries_)e.busy=false;if(op!=Op::Cancel){latestPending_[0]=latestPending_[1]=false;stopped_=true;}}
            else if(!statusAt_||now-statusAt_>=2000||!body(status_)[3]||!credits())return false;
        }
        if(lastAck_&&!maplink::forward(seq,lastAck_))return false;
        for(auto& e:entries_)if(!e.busy){e.message=m;e.seq=seq;e.nav=nav;e.ticket=++ticket_;e.acceptedAt=now;e.busy=true;e.sent=e.acked=false;highWater_=std::max(highWater_,window_-credits());dirty_=true;return true;}
        return false;
    }
    bool take(Message& out,bool observation){
        if(observation){for(unsigned n=0;n<2;++n){const unsigned i=(latestTurn_+n)%2;if(latestPending_[i]){out=latest_[i];latestPending_[i]=false;latestTurn_=1-i;return true;}}return false;}
        Entry* next=nullptr;for(auto& e:entries_)if(e.busy&&!e.sent&&(!next||e.ticket<next->ticket))next=&e;
        if(!next)return false;
        out=next->message;next->sent=true;return true;
    }
    bool urgent(){for(const auto& e:entries_)if(e.busy&&!e.sent&&(e.message.bytes[5]==1||e.message.bytes[5]==7||e.message.bytes[5]==11||e.message.bytes[5]==13))return true;return false;}
    void retryNotification(unsigned kind){if(kind==65)interestDirty_=true;else if(kind==66)routeNeedDirty_=true;else dirty_=true;}
    bool takeDirty(){const bool v=dirty_;dirty_=false;return v;}
    bool takeInterest(Message& m){if(!interestDirty_)return false;interestDirty_=false;m=interest_;return true;}
    bool takeRouteNeed(Message& m){if(!routeNeedDirty_)return false;routeNeedDirty_=false;m=routeNeed_;return true;}
private:
    struct Entry {Message message;std::uint32_t seq=0,nav=0;std::uint64_t ticket=0,acceptedAt=0;bool busy=false,sent=false,acked=false;};
    std::array<Entry,4> entries_{};std::array<Message,2> latest_{};bool latestPending_[2]{};std::uint64_t latestAt_[2]{};unsigned latestTurn_=0,window_=4;
    Capability cap_{};Message status_,interest_,routeNeed_;std::uint64_t capAt_=0,statusAt_=0,ticket_=0;
    std::uint32_t vehicle_=0,nav_=0,lastAck_=0;bool dirty_=false,interestDirty_=false,routeNeedDirty_=false,stopped_=false;
    unsigned maxAckMs_=0,highWater_=0;
};
}
