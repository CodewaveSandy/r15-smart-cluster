#pragma once
#include <cstdint>
namespace cluster::inputs {
// A released level must settle before the first press is accepted after boot.
class Button {
    bool candidate_=false,stable_=false,armed_=false;
    std::uint32_t since_=0;
public:
    bool update(bool pressed,std::uint32_t now) {
        if(pressed!=candidate_) {candidate_=pressed;since_=now;}
        if(std::uint32_t(now-since_)<40)return false;
        if(!candidate_)armed_=true;
        if(stable_==candidate_)return false;
        stable_=candidate_;
        if(stable_&&armed_) {armed_=false;return true;}
        return false;
    }
};
}
