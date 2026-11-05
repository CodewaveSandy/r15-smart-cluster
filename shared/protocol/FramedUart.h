#pragma once
#include "Binary.h"
#include <array>
#include <algorithm>
namespace cluster::uartrecords {
constexpr unsigned maxInner=224;
struct Message {std::array<std::uint8_t,maxInner> bytes{};unsigned size=0;};
struct WireRecord {std::array<std::uint8_t,133> bytes{};unsigned size=0;};
inline WireRecord fragment(const std::uint8_t* b,unsigned total,unsigned direction,unsigned sequence,unsigned offset){
    WireRecord out;if(!total||total>224||offset>=total)return out;
    std::array<std::uint8_t,64> raw{};const auto n=std::min(48u,total-offset);raw[0]=1;raw[1]=std::uint8_t(direction);binary::put(raw.data(),2,sequence,2);binary::put(raw.data(),4,total,2);binary::put(raw.data(),6,offset,2);raw[8]=std::uint8_t(n);std::copy_n(b+offset,n,raw.data()+12);binary::seal(raw.data(),n+16);
    out.bytes[0]='R';out.bytes[1]='1';out.bytes[2]='5';out.bytes[3]='Q';out.size=4;
    for(unsigned i=0;i<n+16;++i){auto v=raw[i];if(v==0||v==0x52||v==0x7d){out.bytes[out.size++]=0x7d;v^=0x20;}out.bytes[out.size++]=v;}
    out.bytes[out.size++]=0;return out;
}
class Assembler {
public:
    void reset(){used_=total_=0;}
    template<class Complete> bool push(const std::uint8_t* b,unsigned n,std::uint64_t now,unsigned direction,Complete complete){
        if(n<17||n>64||b[0]!=1||b[1]!=direction||b[8]<1||b[8]>48||n!=unsigned(16+b[8])||!binary::zero(b,9,3)||!binary::intact(b,n))return false;
        const unsigned seq=unsigned(binary::get(b,2,2)),total=unsigned(binary::get(b,4,2)),offset=unsigned(binary::get(b,6,2)),length=b[8];
        if(!total||total>224||offset>=total||length>total-offset)return false;
        if(used_&&(now<started_||now-started_>500))reset();
        if(!offset){
            if(used_&&seq==sequence_&&total==total_&&length<=used_&&std::equal(b+12,b+12+length,buffer_.begin()))return false;
            used_=0;total_=total;sequence_=seq;started_=now;
        }
        if(seq!=sequence_||total!=total_||now<started_||now-started_>500)return false;
        if(offset<used_){if(length<=used_-offset&&std::equal(b+12,b+12+length,buffer_.begin()+offset))return false;reset();return false;}
        if(offset!=used_){reset();return false;}
        std::copy_n(b+12,length,buffer_.begin()+offset);used_+=length;
        if(used_!=total_)return false;
        complete(buffer_.data(),total_,now);reset();return true;
    }
private:std::array<std::uint8_t,224> buffer_{};unsigned used_=0,total_=0,sequence_=0;std::uint64_t started_=0;
};
}
