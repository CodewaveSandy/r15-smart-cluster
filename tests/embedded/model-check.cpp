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
    for(auto member:{&ClusterState::realtimeFuelLPer100Km,&ClusterState::averageFuelLPer100Km,&ClusterState::averageSpeedKph}) {
        for(double value:{-1.0,0.0,1.0,400.0,1001.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
            s={}; s.*member={value,Quality::Available};
            const double maximum=member==&ClusterState::averageSpeedKph?400.0:1000.0;
            check(valid(s)==(std::isfinite(value)&&value>=0&&value<=maximum),"OEM ride observation bounds");
        }
    }
    Lifecycle l; check(l.phase()==Phase::Off,"Initial off"); l.key(true,100);
    l.advance(3099); check(l.phase()==Phase::Boot,"Boot cannot finish early");
    l.key(true,3080); l.advance(3100); check(l.phase()==Phase::Running,"Repeated on does not restart boot");
    l.key(false,4000); l.advance(5999); check(l.phase()==Phase::Shutdown,"Shutdown cannot finish early");
    l.key(true,6000); check(l.phase()==Phase::Running,"Key-on cancels shutdown");
    l.key(false,7000); l.advance(9000); check(l.phase()==Phase::ReadyForPowerOff,"Shutdown completes");
    l.key(true,10000); check(l.phase()==Phase::Boot,"New power cycle boots");
    s={}; s.engineWarning={true,Quality::Available}; check(urgent(s),"Valid urgent warning");
    s.engineWarning.quality=Quality::Stale; check(!urgent(s),"Stale flag not treated as current warning");
    check(Lifecycle::bootMs==3000&&Lifecycle::shutdownMs==2000,"Animation duration requirement");
    using namespace cluster::presentation;
    for(int value:{0,1,6,9,10,12,13,15,16,17,19,20,21,50,72,99,100}) {
        auto on=fuelPresentation({value,Quality::Available},0);
        auto off=fuelPresentation({value,Quality::Available},500);
        check(on.lit,"Fuel starts lit"); check(off.lit==(value>=20),"Fuel blink threshold");
        check(on.severity==(value<10?FuelSeverity::Critical:value<=20?FuelSeverity::Warning:FuelSeverity::Normal),"Fuel severity boundaries");
        int coverage=0;
        for(int i=0;i<Policy::fuelSegments;++i) {
            const int fill=on.segmentFill(i);
            check(fill>=0&&fill<=100,"Fuel coverage bounded"); coverage+=fill;
        }
        check(coverage==value*8,"Fuel retains fractional segment coverage");
        check(on.reserve==(value<=16),"Reserve threshold from supplied capacities");
    }
    check(fuelPresentation({6,Quality::Available},0).segmentFill(0)==48,"Six percent gives roughly half of first segment");
    check(!fuelPresentation({},0).reserve && !fuelPresentation({10,Quality::Stale},0).reserve,"Unknown fuel never asserts reserve");
    check(fuelPresentation({},0).severity==FuelSeverity::Unknown,"Unknown fuel is not empty");
    for(int rpm:{10499,10500,11399,11400}) {
        check(shiftLit({rpm,Quality::Available},0)==(rpm>=10500),"Shift threshold");
        check(shiftLit({rpm,Quality::Available},300)==(rpm>=10500&&rpm<11400),"Shift flash boundary");
    }
    check(!shiftLit({13000,Quality::Stale},0),"Stale RPM cannot activate shift");
    TripPage trip=TripPage::Odometer; InfoPage info=InfoPage::Coolant;
    for(int i=0;i<3;++i) trip=next(trip);
    check(trip==TripPage::Odometer&&info==InfoPage::Coolant,"Trip cycles independently and wraps");
    for(int i=0;i<7;++i) info=next(info);
    check(info==InfoPage::Coolant,"Info cycles and wraps");
    s={}; s.odometerKm={100,Quality::Available}; s.trip1Km={10,Quality::Available}; s.trip2Km={20,Quality::Available};
    check(tripReading(s,TripPage::Odometer).value==100,"ODO reading");
    check(tripReading(s,TripPage::Trip1).value==10,"Trip A reading");
    check(tripReading(s,TripPage::Trip2).value==20,"Trip B reading");
    s.trip1Km={-1,Quality::Available}; check(!valid(s),"Negative trip rejected");
    s.trip1Km={std::numeric_limits<double>::infinity(),Quality::Available}; check(!valid(s),"Infinite trip rejected");
    check(std::strcmp(lapText({0,Quality::Available}).value,"00:00.00")==0,"Zero lap is a valid duration");
    check(std::strcmp(lapText({59999,Quality::Available}).value,"00:59.99")==0,"Lap hundredths truncate");
    check(std::strcmp(lapText({60000,Quality::Available}).value,"01:00.00")==0,"Minute rollover");
    check(std::strcmp(lapText({84380,Quality::Available}).value,"01:24.38")==0,"Lap formatting");
    check(std::strcmp(lapText({5999999,Quality::Available}).value,"99:59.99")==0,"Maximum displayed lap");
    check(std::strcmp(lapText({6000000,Quality::Available}).value,"—:—.—")==0,"Lap overflow does not wrap");
    check(std::strcmp(lapText({84380,Quality::Stale}).value,"—:—.—")==0,"Stale lap is not live elapsed time");
    return failures?1:0;
}
