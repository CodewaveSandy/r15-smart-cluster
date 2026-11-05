#pragma once
#include "FramedUart.h"
namespace cluster::uartrecords {
// One binary/text demultiplexer. Corrupt framed bytes never enter legacy decoders.
class Receiver {
public:
    struct Diagnostics {unsigned fragments=0,framingDrops=0,crcDrops=0,timeouts=0;};
    const Diagnostics& diagnostics()const{return diagnostics_;}
    void reset(){used_=expected_=matched_=0;tunnel_=drop_=escape_=false;}
    template<class Record> void push(std::uint8_t byte,std::uint64_t now,Record record){push(byte,now,record,[](const std::uint8_t*,unsigned,std::uint64_t){});}
    template<class Record,class Fragment> void push(std::uint8_t byte,std::uint64_t now,Record record,Fragment fragment){
        if((matched_||expected_||tunnel_)&&now-started_>500){++diagnostics_.timeouts;reset();}
        if(tunnel_){
            if(byte==0){
                if(drop_||escape_||used_<17||buffer_[0]!=1||buffer_[1]<1||buffer_[1]>2||buffer_[8]<1||buffer_[8]>48||used_!=unsigned(16+buffer_[8])||!binary::zero(buffer_.data(),9,3))++diagnostics_.framingDrops;
                else if(!binary::intact(buffer_.data(),used_))++diagnostics_.crcDrops;
                else {++diagnostics_.fragments;fragment(buffer_.data(),used_,now);}
                reset();return;
            }
            if(drop_)return;
            if(escape_){if(byte!=0x20&&byte!=0x72&&byte!=0x5d){drop_=true;return;}byte^=0x20;escape_=false;}
            else if(byte==0x7d){escape_=true;return;}
            else if(byte==0x52){drop_=true;return;}
            if(used_>=64){drop_=true;return;}buffer_[used_++]=byte;return;
        }
        if(expected_){buffer_[used_++]=byte;if(used_==expected_){record(buffer_.data(),used_,now);reset();}return;}
        constexpr char prefix[]="R15";
        if(matched_<3){
            if(byte==std::uint8_t(prefix[matched_])){if(!matched_)started_=now;buffer_[matched_++]=byte;}
            else {matched_=byte=='R'?1:0;if(matched_){buffer_[0]=byte;started_=now;}}
            return;
        }
        if(byte=='Q'){tunnel_=true;used_=0;return;}
        const unsigned length=byte=='G'?36:byte=='P'?48:byte=='N'?64:byte=='M'?80:0;
        if(length){buffer_[3]=byte;used_=4;expected_=length;return;}
        matched_=byte=='R'?1:0;if(matched_){buffer_[0]=byte;started_=now;}
    }
private:
    Diagnostics diagnostics_;std::array<std::uint8_t,80> buffer_{};
    unsigned used_=0,expected_=0,matched_=0;std::uint64_t started_=0;bool tunnel_=false,drop_=false,escape_=false;
};
}
