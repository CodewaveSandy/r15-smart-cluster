#pragma once
#include <array>
#include <cstdint>

namespace cluster::uartrecords {
// Isolate complete binary records from console text. Payload CRC, identity and
// freshness remain the responsibility of the existing per-record decoders.
class Receiver {
public:
    void reset(){used_=expected_=matched_=0;retired_=false;}
    template<class Record> void push(std::uint8_t byte,std::uint64_t now,Record record){
        if((matched_||expected_||retired_)&&now-started_>500)reset();
        // Ignore escaped records from the retired firmware, including payloads
        // containing legacy magic. They are never interpreted as live controls.
        if(retired_){if(byte==0)reset();return;}
        if(expected_){buffer_[used_++]=byte;if(used_==expected_){record(buffer_.data(),used_,now);reset();}return;}
        constexpr char prefix[]="R15";
        if(matched_<3){
            if(byte==std::uint8_t(prefix[matched_])){if(!matched_)started_=now;buffer_[matched_++]=byte;}
            else {matched_=byte=='R'?1:0;if(matched_){buffer_[0]=byte;started_=now;}}
            return;
        }
        if(byte=='Q'){retired_=true;return;}
        const unsigned length=byte=='G'?36:byte=='P'?48:byte=='N'?64:byte=='M'?80:0;
        if(length){buffer_[3]=byte;used_=4;expected_=length;return;}
        matched_=byte=='R'?1:0;if(matched_){buffer_[0]=byte;started_=now;}
    }
private:
    std::array<std::uint8_t,80> buffer_{};
    unsigned used_=0,expected_=0,matched_=0;std::uint64_t started_=0;bool retired_=false;
};
}
