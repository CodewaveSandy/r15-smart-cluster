#include "embedded/presentation/DashboardPresentation.h"
#include "embedded/application/DashboardInput.h"
#include "embedded/presentation/SignalChange.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>

int main() {
    using namespace cluster;
    using namespace cluster::presentation;
    int failures=0;
    const auto check=[&](bool ok,const char* why) { if(!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    const auto is=[](const char* a,const char* b) { return std::strcmp(a,b)==0; };
    ClusterState state; DisplayOptions options;
    auto header=headerStatus(state,options,false);
    check(is(gearText(state.gear).value,"—")&&!header.neutral&&header.unavailable,"Unknown gear is not neutral");
    check(is(phoneBatteryText(state.phoneConnected,state.phoneBatteryPercent).value,"PHONE —"),"Unknown phone battery");
    for(auto quality:{Quality::Unavailable,Quality::Unsupported,Quality::Invalid,Quality::Stale}) {
        state.speedKph={60,quality}; state.gear={0,quality};
        check(is(primaryPresentation(state,options).speed.value,"—"),"Non-current speed hidden");
        check(!headerStatus(state,options,false).neutral,"Non-current neutral is not active");
        state.batteryVoltageV={12.8,quality}; options.infoPage=InfoPage::Battery;
        check(is(rideInfoPresentation(state,options).infoValue.value,"— V"),"Non-current voltage hidden");
    }
    state={}; state.speedKph={60,Quality::Available}; state.engineRpm={8500,Quality::Available}; state.gear={0,Quality::Available};
    check(is(gearText(state.gear).value,"N"),"Neutral formatting");
    for(int gear=1;gear<=6;++gear) {
        state.gear.value=gear; const auto value=gearText(state.gear);
        check(value.value[0]=='0'+gear&&value.value[1]=='\0',"Gear formatting");
    }
    for(int speed:{0,1,60,400}) {
        state.speedKph.value=speed;
        check(is(primaryPresentation(state,options).speed.value,formatted("%d",speed).value),"Metric speed boundaries");
    }
    state.speedKph.value=60; options.imperial=true;
    auto primary=primaryPresentation(state,options);
    check(is(primary.speed.value,"37")&&is(primary.unit,"mph"),"Imperial speed rounding and unit");
    state.tripAKm={100,Quality::Available}; options.tripPage=TripPage::TripA;
    auto ride=rideInfoPresentation(state,options);
    check(is(ride.tripValue.value,"62.1")&&is(ride.tripUnit,"mi")&&is(ride.tripTitle,"TRIP A"),"Imperial trip formatting");
    state.bestLapMs={78420,Quality::Available}; options.track=true;
    ride=rideInfoPresentation(state,options);
    check(is(ride.tripValue.value,"01:18.42")&&is(ride.tripTitle,"BEST LAP")&&!ride.showDistance,"Track best lap replaces distance");
    state.batteryVoltageV={12.8,Quality::Available}; options.infoPage=InfoPage::Battery;
    check(is(rideInfoPresentation(state,options).infoValue.value,"12.8 V"),"Voltage formatting");
    options.infoPage=InfoPage::Coolant; state.coolantTemperatureC={84,Quality::Available};
    check(is(rideInfoPresentation(state,options).infoValue.value,"84°C"),"Coolant formatting");
    options.infoPage=InfoPage::Tcs; state.tcsEnabled={false,Quality::Available};
    check(is(rideInfoPresentation(state,options).infoValue.value,"OFF"),"Known false TCS");
    state.phoneConnected={false,Quality::Available}; state.phoneBatteryPercent={84,Quality::Available};
    check(is(phoneBatteryText(state.phoneConnected,state.phoneBatteryPercent).value,"PHONE —"),"Disconnected phone suppresses battery");
    state.phoneConnected.value=true;
    check(is(phoneBatteryText(state.phoneConnected,state.phoneBatteryPercent).value,"PHONE 84%"),"Connected phone battery");
    check(is(formatted("%040d",1).value,"—"),"Formatting overflow has explicit unavailable output");
    check(is(rpmText({8500,Quality::Available}).value,"8500 rpm")&&is(rpmText({}).value,"— rpm"),"RPM formatting");
    check(is(fuelText({0,Quality::Available}).value,"0%")&&is(fuelText({}).value,"—"),"Unknown fuel differs from zero");
    check(lampPresentation({true,Quality::Stale},true,false).tone==LampTone::Unknown,"Stale lamp is unknown");
    check(lampPresentation({false,Quality::Available},true,false).opacity==31,"Known inactive lamp");
    check(lampPresentation({true,Quality::Available},false,false).opacity==31,"Blink off phase");
    check(lampPresentation({},false,true).opacity==255,"Icon test override");

    SignalChange<int> changes;
    check(changes.update({}),"First presentation input initializes");
    check(!changes.update({}),"Repeated unknown input is stable");
    check(!changes.update({123,Quality::Unavailable}),"Unknown payload changes do not reformat");
    check(changes.update({0,Quality::Available}),"Unknown to valid zero refreshes");
    check(!changes.update({0,Quality::Available}),"Unchanged zero does not reformat");
    check(changes.update({1,Quality::Available}),"New valid number refreshes");
    check(changes.update({1,Quality::Stale}),"Quality change refreshes");
    check(changes.update({1,Quality::Available}),"Recovered value refreshes even with same payload");

    options={};
    for(int i=0;i<4;++i) applyAction(options,DashboardAction::NextTrip);
    check(options.tripPage==TripPage::Odometer&&options.infoPage==InfoPage::Coolant,"Trip wraps independently");
    applyAction(options,DashboardAction::NextTrip);
    applyAction(options,DashboardAction::ToggleLayout);
    applyAction(options,DashboardAction::NextTrip);
    check(options.tripPage==TripPage::TripA,"Track suppresses trip cycling");
    applyAction(options,DashboardAction::ToggleLayout);
    check(options.tripPage==TripPage::TripA,"Street trip retained after Track");
    for(int i=0;i<4;++i) applyAction(options,DashboardAction::NextInfo);
    check(options.infoPage==InfoPage::Coolant,"Info wraps independently");
    check(applyAction(options,DashboardAction::ActivateInfo)==InputEffect::None,"Read-only info has no action");
    options.infoPage=InfoPage::Tcs;
    check(applyAction(options,DashboardAction::ActivateInfo)==InputEffect::RequestTcsToggle,"TCS produces request only");
    options.infoPage=InfoPage::Track;
    check(applyAction(options,DashboardAction::ActivateInfo)==InputEffect::None&&options.track,"Track activation changes display locally");
    applyAction(options,DashboardAction::ToggleTheme); applyAction(options,DashboardAction::ToggleUnits);
    check(options.light&&options.imperial,"Local preferences toggle");
    const auto tripBefore=options.tripPage;
    applyAction(options,static_cast<DashboardAction>(999));
    check(options.tripPage==tripBefore&&options.light&&options.imperial,"Unknown action leaves preferences unchanged");
    return failures?1:0;
}
