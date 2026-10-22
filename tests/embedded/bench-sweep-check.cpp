#include "firmware/display/main/BenchSweep.h"
#include "firmware/display/main/BenchTouch.h"
#include "embedded/presentation/InstrumentPresentation.h"
#include <cassert>
#include <limits>
int main() {
    using cluster::board::benchSweep;
    assert(benchSweep(0).speedKph.value==0);
    assert(benchSweep(10000).engineRpm.value==13000);
    assert(benchSweep(11999).engineRpm.value==13000);
    assert(benchSweep(22000).speedKph.value==0);
    assert(benchSweep(24000).engineRpm.value==0);
    bool steady=false,flashOn=false,flashOff=false;
    for(std::uint64_t t=0;t<=48000;t+=10) {
        auto s=benchSweep(t);
        assert(cluster::valid(s));
        assert(!s.odometerKm.available() && !s.phoneConnected.available());
        const bool lit=cluster::presentation::shiftLit(s.engineRpm,t);
        if(s.engineRpm.value>=10500 && s.engineRpm.value<11400) { assert(lit); steady=true; }
        if(s.engineRpm.value>=11400) { flashOn|=lit; flashOff|=!lit; }
        if(t<10000) assert(s.engineRpm.value<=benchSweep(t+10).engineRpm.value);
        if(t>=12000 && t<22000) assert(s.engineRpm.value>=benchSweep(t+10).engineRpm.value);
    }
    assert(steady&&flashOn&&flashOff);
    assert(cluster::valid(benchSweep(std::numeric_limits<std::uint64_t>::max())));
    assert(benchSweep(0).fuelPercent.value==100);
    assert(benchSweep(16000).fuelPercent.value==20);
    assert(benchSweep(16800).fuelPercent.value==16);
    assert(benchSweep(18000).fuelPercent.value==10);
    assert(benchSweep(18200).fuelPercent.value==9);
    assert(benchSweep(20000).fuelPercent.value==0);
    assert(benchSweep(23999).fuelPercent.value==0);
    assert(benchSweep(44000).fuelPercent.value==100);
    for(unsigned t=0;t<48000;++t) assert(cluster::valid(benchSweep(t)));
    assert(benchSweep(0).left.value && !benchSweep(0).right.value);
    assert(benchSweep(3000).right.value);
    assert(benchSweep(6000).highBeam.value);
    assert(benchSweep(9000).absWarning.value);
    assert(benchSweep(12000).engineWarning.value);
    assert(benchSweep(15000).coolantWarning.value);
    assert(benchSweep(18000).left.value && benchSweep(18000).right.value);
    assert(!cluster::board::benchLampTest(20999));
    assert(cluster::board::benchLampTest(21000));
    assert(cluster::board::benchLampTestLit(21600));
    assert(!cluster::board::benchLampTestLit(22050));
    using cluster::Phase;
    using cluster::board::benchTouchAction;
    assert(benchTouchAction(Phase::Running,700,440)=='o');
    assert(benchTouchAction(Phase::Running,400,240)=='d');
    assert(benchTouchAction(Phase::Shutdown,400,240)==0);
    assert(benchTouchAction(Phase::ReadyForPowerOff,400,240)=='b');
    assert(benchTouchAction(Phase::Boot,700,440)==0);
    assert(benchTouchAction(Phase::Running,800,480)==0);
    cluster::Lifecycle life; life.key(true,0); life.advance(3000);
    life.key(false,4000); life.advance(5999); assert(life.phase()==Phase::Shutdown);
    life.advance(6000); assert(life.phase()==Phase::ReadyForPowerOff);
}
