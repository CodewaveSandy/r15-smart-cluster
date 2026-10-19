#include "embedded/presentation/DashboardPresentation.h"
#include "embedded/application/DashboardInput.h"
#include "embedded/presentation/SignalChange.h"
#include "embedded/application/SettingsNavigation.h"
#include "embedded/presentation/SettingsTransition.h"
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
        state.averageFuelLPer100Km={2.8,quality}; options.infoPage=InfoPage::AverageFuel;
        check(is(rideInfoPresentation(state,options).infoValue.value,"— L/100km"),"Non-current consumption hidden");
    }
    for(auto quality:{Quality::Unavailable,Quality::Unsupported,Quality::Invalid,Quality::Stale}) {
        state.realtimeFuelLPer100Km={3.4,quality}; state.averageFuelLPer100Km={2.8,quality}; state.averageSpeedKph={47.2,quality};
        for(auto page:{InfoPage::RealtimeFuel,InfoPage::AverageFuel,InfoPage::AverageSpeed}) {
            options.infoPage=page;
            const auto value=rideInfoPresentation(state,options).infoValue;
            check(is(value.value,page==InfoPage::AverageSpeed?"— km/h":"— L/100km"),"Non-current OEM info hidden");
        }
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
    state.trip1Km={100,Quality::Available}; options.tripPage=TripPage::Trip1;
    auto ride=rideInfoPresentation(state,options);
    check(is(ride.tripValue.value,"62.1")&&is(ride.tripUnit,"mi")&&is(ride.tripTitle,"TRIP 1"),"Imperial trip formatting");
    state.bestLapMs={78420,Quality::Available}; options.track=true;
    ride=rideInfoPresentation(state,options);
    check(is(ride.tripValue.value,"01:18.42")&&is(ride.tripTitle,"BEST LAP")&&!ride.showDistance,"Track best lap replaces distance");
    state.averageFuelLPer100Km={2.8,Quality::Available}; options.infoPage=InfoPage::AverageFuel;
    check(is(rideInfoPresentation(state,options).infoValue.value,"2.8 L/100km"),"Consumption formatting");
    options.infoPage=InfoPage::RealtimeFuel; state.realtimeFuelLPer100Km={3.4,Quality::Available};
    check(is(rideInfoPresentation(state,options).infoValue.value,"3.4 L/100km"),"Real-time consumption formatting");
    options.infoPage=InfoPage::AverageSpeed; state.averageSpeedKph={47.2,Quality::Available}; options.imperial=false;
    check(is(rideInfoPresentation(state,options).infoValue.value,"47.2 km/h"),"Average speed formatting");
    options.infoPage=InfoPage::Unidentified;
    check(is(rideInfoPresentation(state,options).infoValue.value,"—"),"Unidentified page never invents a reading");
    options.infoPage=InfoPage::Coolant; state.coolantTemperatureC={84,Quality::Available};
    check(is(rideInfoPresentation(state,options).infoValue.value,"84°C"),"Coolant formatting");
    options.infoPage=InfoPage::Tcs; state.tcsEnabled={false,Quality::Available};
    check(is(rideInfoPresentation(state,options).infoValue.value,"OFF"),"Known false TCS");
    state.phoneConnected={false,Quality::Available}; state.phoneBatteryPercent={84,Quality::Available};
    check(is(phoneBatteryText(state.phoneConnected,state.phoneBatteryPercent).value,"PHONE —"),"Disconnected phone suppresses battery");
    state.phoneConnected.value=true;
    check(is(phoneBatteryText(state.phoneConnected,state.phoneBatteryPercent).value,"PHONE 84%"),"Connected phone battery");
    check(is(formatted("%040d",1).value,"—"),"Formatting overflow has explicit unavailable output");
    check(is(rpmNumberText({8500,Quality::Available}).value,"8,500")&&is(rpmNumberText({}).value,"—"),"RPM formatting");
    check(is(rpmNumberText({0,Quality::Available}).value,"0") &&
          is(rpmNumberText({999,Quality::Available}).value,"999") &&
          is(rpmNumberText({1000,Quality::Available}).value,"1,000") &&
          is(rpmNumberText({13000,Quality::Available}).value,"13,000") &&
          is(rpmNumberText({8500,Quality::Stale}).value,"—"),"RPM grouping boundaries and stale data");
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
    for(auto page:{InfoPage::Coolant,InfoPage::Track,InfoPage::RealtimeFuel,InfoPage::AverageFuel,InfoPage::AverageSpeed,InfoPage::Tcs,InfoPage::Unidentified}) {
        check(options.infoPage==page,"OEM info order");
        applyAction(options,DashboardAction::NextInfo);
    }
    check(options.infoPage==InfoPage::Coolant,"OEM info returns to coolant");
    for(int i=0;i<3;++i) applyAction(options,DashboardAction::NextTrip);
    check(options.tripPage==TripPage::Odometer&&options.infoPage==InfoPage::Coolant,"Trip wraps independently");
    applyAction(options,DashboardAction::NextTrip);
    applyAction(options,DashboardAction::ToggleLayout);
    applyAction(options,DashboardAction::NextTrip);
    check(options.tripPage==TripPage::Trip1,"Track suppresses trip cycling");
    applyAction(options,DashboardAction::ToggleLayout);
    check(options.tripPage==TripPage::Trip1,"Street trip retained after Track");
    for(int i=0;i<7;++i) applyAction(options,DashboardAction::NextInfo);
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
    DisplayOptions menu;
    ClusterState parked; parked.speedKph={0,Quality::Available}; Lifecycle running;
    running.key(true,0); running.advance(Lifecycle::bootMs);
    settingsAction(menu,SettingsAction::Open,parked,running);
    check(menu.settings,"Stationary settings entry");
    const auto savedTrip=menu.tripPage;
    applyAction(menu,DashboardAction::NextTrip);
    check(menu.tripPage==savedTrip,"Settings suppresses hidden dashboard actions");
    settingsAction(menu,SettingsAction::Right,parked,running);
    check(menu.settingsFocus==1,"Right selects Bluetooth");
    settingsAction(menu,SettingsAction::Down,parked,running);
    check(menu.settingsFocus==2,"Down advances to Wi-Fi");
    settingsAction(menu,SettingsAction::Next,parked,running);
    check(menu.settingsFocus==3,"Next advances to Navigation");
    settingsAction(menu,SettingsAction::Right,parked,running);
    check(menu.settingsFocus==0,"Settings sequential navigation wraps");
    parked.speedKph={20,Quality::Available}; reconcileSettings(menu,parked,running);
    check(!menu.settings,"Limit closes settings");
    settingsAction(menu,SettingsAction::Open,parked,running);
    check(!menu.settings,"Limit blocks entry");
    for(int speed:{-1,0,1,19,20,21,400}) {
        menu.settings=false; parked.speedKph={speed,Quality::Available};
        settingsAction(menu,SettingsAction::Open,parked,running);
        check(menu.settings==(speed>=0&&speed<20),"Settings speed boundary");
    }
    menu.settings=true; parked.speedKph={19,Quality::Available}; reconcileSettings(menu,parked,running);
    check(menu.settings,"Settings stays open below limit");
    parked.speedKph.value=20; reconcileSettings(menu,parked,running);
    check(!menu.settings,"Crossing 20 km/h closes settings");
    menu.settings=true; menu.settingsFocus=0;
    settingsAction(menu,SettingsAction::Left,parked,running);
    check(menu.settingsFocus==3,"Left wraps to previous tile");
    settingsAction(menu,SettingsAction::Up,parked,running);
    check(menu.settingsFocus==2,"Up selects previous tile");
    menu.settings=false;
    for(auto quality:{Quality::Unavailable,Quality::Stale,Quality::Invalid,Quality::Unsupported}) {
        parked.speedKph={0,quality}; settingsAction(menu,SettingsAction::Open,parked,running);
        check(!menu.settings,"Unknown speed blocks entry");
    }
    parked.speedKph={0,Quality::Available}; parked.engineWarning={true,Quality::Available};
    settingsAction(menu,SettingsAction::Open,parked,running);
    check(!menu.settings,"Urgent warning blocks settings");
    parked.engineWarning={false,Quality::Available};
    Lifecycle off; settingsAction(menu,SettingsAction::Open,parked,off);
    check(!menu.settings,"Power-off blocks settings");
    SettingsTransition transition;
    check(transition.update(true,100)==0,"Transition starts at dashboard");
    const int half=transition.update(true,240);
    check(half>0&&half<1000,"Transition has intermediate position");
    check(transition.update(false,240)==half,"Reversal preserves current position");
    check(transition.update(false,520)==0,"Reversal returns to dashboard");
    transition.update(true,600); check(transition.update(true,880)==1000,"Transition settles in 280ms");
    check(transition.update(false,881,true)==0,"Warning bypasses transition immediately");
    return failures?1:0;
}
