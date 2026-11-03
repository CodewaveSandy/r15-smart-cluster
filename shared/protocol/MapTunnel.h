#pragma once
#include "MapProtocol.h"

namespace cluster::maplink {
// One demultiplexer isolates complete legacy frames and escaped map records.
// Callbacks receive complete frames only. Untrusted map bytes never enter the
// legacy sliding decoders, even when a tunnel CRC/length check fails.
class Demux {
public:
    void reset(){used_=expected_=matched_=0;tunnel_=drop_=escape_=false;}
    template<class Legacy,class Fragment> void push(std::uint8_t byte,std::uint64_t now,Legacy legacy,Fragment fragment){
        if((used_||tunnel_)&&now-started_>500)reset();
        if(tunnel_){
            if(byte==0){if(!drop_&&!escape_&&used_>=17&&buffer_[0]==1&&buffer_[1]>=1&&buffer_[1]<=2&&buffer_[8]>=1&&buffer_[8]<=48&&used_==16u+buffer_[8]&&zero(buffer_.data(),9,3)&&intact(buffer_.data(),used_))fragment(buffer_.data(),used_,now);reset();return;}
            if(drop_)return;
            if(escape_){if(byte!=0x20&&byte!=0x72&&byte!=0x5d){drop_=true;return;}byte^=0x20;escape_=false;}
            else if(byte==0x7d){escape_=true;return;}
            else if(byte==0x52){drop_=true;return;}
            if(used_>=64){drop_=true;return;}buffer_[used_++]=byte;return;
        }
        if(expected_){buffer_[used_++]=byte;if(used_==expected_){legacy(buffer_.data(),used_,now);reset();}return;}
        const char prefix[]="R15";
        if(matched_<3){if(byte==std::uint8_t(prefix[matched_])){if(!matched_)started_=now;buffer_[matched_++]=byte;}else matched_=byte=='R'?1:0;return;}
        if(byte=='Q'){tunnel_=true;used_=0;return;}
        const unsigned length=byte=='G'?36:byte=='P'?48:byte=='N'?64:byte=='M'?80:0;
        if(length){buffer_[3]=byte;used_=4;expected_=length;return;}
        matched_=byte=='R'?1:0;
    }
private:
    std::array<std::uint8_t,80> buffer_{};unsigned used_=0,expected_=0,matched_=0;std::uint64_t started_=0;bool tunnel_=false,drop_=false,escape_=false;
};
struct Assembly {
    Message message;unsigned used=0;std::uint16_t sequence=0;std::uint64_t at=0;
    void reset(){used=0;message.size=0;}
};
class Assembler {
public:
    void reset(){for(auto& s:slots_)s.reset();}
    // Returns true for a completed inner message. No persistent payload logging.
    bool push(const std::uint8_t* b,unsigned,std::uint64_t now,unsigned direction,Message& output){
        if(b[1]!=direction)return false;
        const auto sequence=std::uint16_t(get(b,2,2));const unsigned total=unsigned(get(b,4,2)),offset=unsigned(get(b,6,2)),length=b[8];
        if(!total||total>240||offset>=total||length>total-offset)return false;
        for(auto& s:slots_)if(s.used&&now-s.at>=2000)s.reset();
        unsigned index=0;
        if(!offset){
            if(length<8)return false;
            if(direction==1){if(!magic(b+12,'X'))return false;index=b[17]==7?1:0;}
            else {if(!magic(b+12,'S')&&!magic(b+12,'K'))return false;index=magic(b+12,'K')?1:0;}
            auto& s=slots_[index];
            if(s.used>=length&&s.sequence==sequence&&s.message.size==total&&std::equal(b+12,b+12+length,s.message.bytes.begin()))return false;
            s.reset();s.sequence=sequence;s.message.size=total;
        }else{
            bool found=false;for(unsigned i=0;i<2;++i)if(slots_[i].used&&slots_[i].sequence==sequence&&slots_[i].message.size==total){index=i;found=true;break;}if(!found)return false;
        }
        auto& s=slots_[index];
        if(s.message.size==total&&s.sequence==sequence&&offset<s.used&&length<=s.used-offset&&std::equal(b+12,b+12+length,s.message.bytes.begin()+offset))return false;
        if(s.message.size!=total||s.sequence!=sequence||s.used!=offset){s.reset();return false;}
        std::copy_n(b+12,length,s.message.bytes.data()+offset);s.used+=length;s.at=now;
        if(s.used!=total)return false;
        output=s.message;s.reset();return true;
    }
private:std::array<Assembly,2> slots_{};
};
struct WireRecord {std::array<std::uint8_t,133> bytes{};unsigned size=0;};
inline WireRecord fragment(const Message& m,unsigned direction,std::uint16_t sequence,unsigned offset){
    WireRecord out;if(!m.size||m.size>240||offset>=m.size)return out;
    std::array<std::uint8_t,64> raw{};const unsigned n=std::min(48u,m.size-offset);
    raw[0]=1;raw[1]=std::uint8_t(direction);put(raw.data(),2,sequence,2);put(raw.data(),4,m.size,2);put(raw.data(),6,offset,2);raw[8]=std::uint8_t(n);std::copy_n(m.bytes.data()+offset,n,raw.data()+12);seal(raw.data(),n+16);
    out.bytes[0]='R';out.bytes[1]='1';out.bytes[2]='5';out.bytes[3]='Q';out.size=4;
    for(unsigned i=0;i<n+16;++i){auto v=raw[i];if(v==0||v==0x52||v==0x7d){out.bytes[out.size++]=0x7d;v^=0x20;}out.bytes[out.size++]=v;}
    out.bytes[out.size++]=0;return out;
}
// Retain a whole logical message across prioritized fragments, never its bytes
// in a driver TX backlog. Caller emits at most one fragment between critical TX.
class Sender {
public:
    explicit Sender(std::uint16_t seed=0):sequence_(seed){}
    bool enqueue(const Message& m){if(pending_||!m.size||m.size>240)return false;message_=m;offset_=0;pending_=true;sequence_=std::uint16_t(sequence_+2);return true;}
    bool pending()const{return pending_;}
    void reset(){pending_=false;offset_=0;}
    WireRecord next(unsigned direction){return pending_?fragment(message_,direction,sequence_,offset_):WireRecord{};}
    void sent(){offset_+=std::min(48u,message_.size-offset_);if(offset_==message_.size)pending_=false;}
    bool bulk()const{return pending_&&magic(message_.bytes.data(),'X')&&message_.bytes[5]==4;}
private:Message message_;unsigned offset_=0;std::uint16_t sequence_=0;bool pending_=false;
};
}
