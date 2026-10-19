#pragma once
#include <cstdint>
#include <algorithm>
namespace cluster::presentation {
// Monotonic, reversible, bounded transition; no timers, callbacks or allocations.
class SettingsTransition {
public:
    static constexpr std::uint64_t durationMs=280;
    int update(bool open,std::uint64_t now,bool immediate=false) {
        if(immediate) { target_=open?1000:0; value_=from_=target_; entered_=now; return value_; }
        sample(now);
        const int target=open?1000:0;
        if(target!=target_) { from_=value_; target_=target; entered_=now; }
        return value_;
    }
private:
    void sample(std::uint64_t now) {
        if(now<entered_) return;
        const auto elapsed=std::min(now-entered_,durationMs);
        const int t=int(elapsed*1000/durationMs);
        const int eased=t*t/1000*(3000-2*t)/1000;
        value_=from_+(target_-from_)*eased/1000;
    }
    std::uint64_t entered_=0;
    int value_=0,from_=0,target_=0;
};
}
