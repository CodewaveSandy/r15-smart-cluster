#pragma once
#include <cstdint>
namespace cluster {
enum class Phase { Off, Boot, Running, Shutdown, ReadyForPowerOff };
class Lifecycle {
public:
    static constexpr std::uint64_t animationMs = 1250;
    void key(bool on, std::uint64_t now) {
        if (on == ignition_) return;
        ignition_ = on;
        if (on) phase_ = phase_ == Phase::Shutdown ? Phase::Running : Phase::Boot;
        else phase_ = phase_ == Phase::Off ? Phase::Off : Phase::Shutdown;
        entered_ = now;
    }
    void advance(std::uint64_t now) {
        if (elapsed(now) < animationMs) return;
        if (phase_ == Phase::Boot) phase_ = Phase::Running;
        else if (phase_ == Phase::Shutdown) phase_ = Phase::ReadyForPowerOff;
    }
    std::uint64_t elapsed(std::uint64_t now) const { return now >= entered_ ? now-entered_ : 0; }
    Phase phase() const { return phase_; }
    bool ignition() const { return ignition_; }
private:
    Phase phase_ = Phase::Off;
    bool ignition_ = false;
    std::uint64_t entered_ = 0;
};
} // namespace cluster
