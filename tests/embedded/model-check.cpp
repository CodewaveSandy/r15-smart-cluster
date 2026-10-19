#include "shared/model/ClusterState.h"
#include "embedded/application/Lifecycle.h"
#include "embedded/application/DashboardPages.h"
#include "embedded/presentation/InstrumentPresentation.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
int main() {
    using namespace cluster; int failures=0;
    auto check=[&](bool condition,const char* reason){if(!condition){std::fprintf(stderr,"FAIL: %s\n",reason);++failures;}};
    ClusterState s; check(!s.speedKph.available(),"Default speed unknown"); check(valid(s),"Unknown snapshot accepted");
    check(!s.bestLapMs.available(),"Best lap starts unavailable, not zero");
    for(int v:{-1,0,60,400,401}) { s.speedKph={v,Quality::Available}; check(valid(s)==(v>=0&&v<=400),"Speed boundaries"); }
    s={}; for(int v:{-1,0,72,100,101}) { s.fuelPercent={v,Quality::Available}; check(valid(s)==(v>=0&&v<=100),"Fuel boundaries"); }
    s={}; s.batteryVoltageV={std::numeric_limits<double>::quiet_NaN(),Quality::Available}; check(!valid(s),"Reject available NaN");
    Lifecycle l; check(l.phase()==Phase::Off,"Initial off"); l.key(true,100);
    l.advance(1349); check(l.phase()==Phase::Boot,"Boot cannot finish early");
    l.key(true,1300); l.advance(1350); check(l.phase()==Phase::Running,"Repeated on does not restart boot");
    l.key(false,2000); l.advance(3249); check(l.phase()==Phase::Shutdown,"Shutdown cannot finish early");
    l.key(true,3250); check(l.phase()==Phase::Running,"Key-on cancels shutdown");
    l.key(false,4000); l.advance(5250); check(l.phase()==Phase::ReadyForPowerOff,"Shutdown completes");
    l.key(true,6000); check(l.phase()==Phase::Boot,"New power cycle boots");
    s={}; s.engineWarning={true,Quality::Available}; check(urgent(s),"Valid urgent warning");
    s.engineWarning.quality=Quality::Stale; check(!urgent(s),"Stale flag not treated as current warning");
    check(Lifecycle::animationMs>=1000&&Lifecycle::animationMs<=1500,"Animation duration requirement");
    using namespace cluster::presentation;
    for(int value:{0,9,10,19,20,21,100}) {
        auto on=fuelPresentation({value,Quality::Available},0);
        auto off=fuelPresentation({value,Quality::Available},500);
        check(on.lit,"Fuel starts lit"); check(off.lit==(value>=20),"Fuel blink threshold");
        check(on.severity==(value<10?FuelSeverity::Critical:value<=20?FuelSeverity::Warning:FuelSeverity::Normal),"Fuel severity boundaries");
        check(on.segments==(value*8+99)/100,"Fuel segment count");
    }
    check(fuelPresentation({},0).severity==FuelSeverity::Unknown,"Unknown fuel is not empty");
    for(int rpm:{10499,10500,11399,11400}) {
        check(shiftLit({rpm,Quality::Available},0)==(rpm>=10500),"Shift threshold");
        check(shiftLit({rpm,Quality::Available},300)==(rpm>=10500&&rpm<11400),"Shift flash boundary");
    }
    check(!shiftLit({13000,Quality::Stale},0),"Stale RPM cannot activate shift");
    TripPage trip=TripPage::Odometer; InfoPage info=InfoPage::Coolant;
    for(int i=0;i<4;++i) trip=next(trip);
    check(trip==TripPage::Odometer&&info==InfoPage::Coolant,"Trip cycles independently and wraps");
    for(int i=0;i<4;++i) info=next(info);
    check(info==InfoPage::Coolant,"Info cycles and wraps");
    s={}; s.odometerKm={100,Quality::Available}; s.tripAKm={10,Quality::Available}; s.tripBKm={20,Quality::Available};
    check(tripReading(s,TripPage::Odometer).value==100,"ODO reading");
    check(tripReading(s,TripPage::TripA).value==10,"Trip A reading");
    check(tripReading(s,TripPage::TripB).value==20,"Trip B reading");
    check(!tripReading(s,TripPage::TripF).available(),"Unavailable Trip F is not zero");
    s.tripAKm={-1,Quality::Available}; check(!valid(s),"Negative trip rejected");
    s.tripAKm={std::numeric_limits<double>::infinity(),Quality::Available}; check(!valid(s),"Infinite trip rejected");
    check(std::strcmp(lapText({0,Quality::Available}).value,"00:00.00")==0,"Zero lap is a valid duration");
    check(std::strcmp(lapText({59999,Quality::Available}).value,"00:59.99")==0,"Lap hundredths truncate");
    check(std::strcmp(lapText({60000,Quality::Available}).value,"01:00.00")==0,"Minute rollover");
    check(std::strcmp(lapText({84380,Quality::Available}).value,"01:24.38")==0,"Lap formatting");
    check(std::strcmp(lapText({5999999,Quality::Available}).value,"99:59.99")==0,"Maximum displayed lap");
    check(std::strcmp(lapText({6000000,Quality::Available}).value,"—:—.—")==0,"Lap overflow does not wrap");
    check(std::strcmp(lapText({84380,Quality::Stale}).value,"—:—.—")==0,"Stale lap is not live elapsed time");
    return failures?1:0;
}
