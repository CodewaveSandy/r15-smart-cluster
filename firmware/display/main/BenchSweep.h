#pragma once
#include "shared/model/ClusterState.h"
#include <algorithm>
#include <cstdint>

namespace cluster::board {
// Desk-only independent visual samples, never a speed/RPM/gear calibration.
// 10 s up, 2 s at redline, 10 s down, 2 s stopped; repeats without drift.
inline ClusterState benchSweep(std::uint64_t elapsedMs) {
    const auto phase=elapsedMs%24000;
    const auto ramp=phase<10000?phase:phase<12000?10000:phase<22000?22000-phase:0;
    ClusterState s;
    s.speedKph={int(ramp*160/10000),Quality::Available};
    s.engineRpm={int(ramp*13000/10000),Quality::Available};
    s.gear={s.speedKph.value==0?0:std::min(6,1+s.speedKph.value/30),Quality::Available};
    // 48 s full-to-empty-to-full cycle, held empty for four seconds so the
    // pump blink remains visible. Includes 20/16/10/9/0 percent boundaries.
    const auto fuelPhase=elapsedMs%48000;
    const int fuel=fuelPhase<20000?100-int(fuelPhase/200):
        fuelPhase<24000?0:fuelPhase<44000?int((fuelPhase-24000)/200):100;
    s.fuelPercent={fuel,Quality::Available};
    s.currentLapMs={std::uint32_t(elapsedMs%600000),Quality::Available};
    s.bestLapMs={78420,Quality::Available};
    s.trip1Km={128.4,Quality::Available}; s.trip2Km={46.8,Quality::Available};
    s.coolantTemperatureC={84,Quality::Available};
    s.left=s.right=s.highBeam=s.absWarning=s.engineWarning=s.coolantWarning={false,Quality::Available};
    switch(phase/3000) {
    case 0: s.left.value=true; break;
    case 1: s.right.value=true; break;
    case 2: s.highBeam.value=true; break;
    case 3: s.absWarning.value=true; break;
    case 4: s.engineWarning.value=true; break;
    case 5: s.coolantWarning.value=true; break;
    case 6: s.left.value=s.right.value=true; break;
    default: break; // Final 3 s is the explicit all-icon lamp test below.
    }
    // Actual odometer, phone and clock remain unavailable, and nothing persists.
    return s;
}
inline bool benchLampTest(std::uint64_t elapsedMs) { return elapsedMs%24000>=21000; }
inline bool benchLampTestLit(std::uint64_t elapsedMs) { return benchLampTest(elapsedMs)&&(elapsedMs/450)%2==0; }
inline ClusterState parkedSettingsFixture() {
    ClusterState s;
    // Only stationary speed is synthesized to exercise the menu gate. No
    // connected gear/neutral, turn phase or other vehicle reading is invented.
    s.speedKph={0,Quality::Available};
    return s;
}
}
