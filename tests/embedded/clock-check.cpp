#include "embedded/presentation/ClockPresentation.h"
#include "embedded/application/SettingsNavigation.h"
#include "embedded/application/Preferences.h"
#include <cassert>
#include <cstring>
#include <string>

int main() {
    using namespace cluster;using namespace presentation;
    Clock clock;assert(!clock.available(0));
    assert(!clock.synchronize(0,330,0));
    assert(!clock.synchronize(Clock::firstEpoch,841,0));
    assert(clock.synchronize(1709164800,330,100)); // 2024-02-29 UTC midnight
    assert(clock.epoch(99)==0&&clock.epoch(1100)==1709164801);
    ClockPreferences prefs;
    auto text=clockText(clock.epoch(100),clock.offset(),prefs);
    assert(std::strcmp(text.time,"05:30")==0&&std::strcmp(text.date,"29/02/2024")==0&&!text.period[0]);
    prefs.twelveHour=true;
    text=clockText(1709164800,0,prefs);assert(std::strcmp(text.time,"12:00")==0&&std::strcmp(text.period,"AM")==0);
    text=clockText(1709208000,0,prefs);assert(std::strcmp(text.time,"12:00")==0&&std::strcmp(text.period,"PM")==0);
    text=clockText(1709249400,0,prefs);assert(std::strcmp(text.time,"11:30")==0&&std::strcmp(text.period,"PM")==0);
    prefs.automaticTimezone=false;prefs.utcOffsetMinutes=-60;prefs.dateFormat=DateFormat::YearMonthDay;
    text=clockText(1709164800,330,prefs);assert(std::strcmp(text.time,"11:00")==0&&std::strcmp(text.date,"2024-02-28")==0);
    prefs.dateFormat=DateFormat::MonthDayYear;text=clockText(1709164800,330,prefs);assert(std::strcmp(text.date,"02/28/2024")==0);
    assert(clock.synchronize(Clock::lastEpoch,0,100));assert(!clock.available(1100));
    ClockConsole console;ClockSync sync;bool ready=false;
    auto feed=[&](const std::string& line){bool accepted=false;for(char c:line){assert(console.consume(c,sync,ready));accepted|=ready;}return accepted;};
    assert(feed("@TIME 1709164800 330\n")&&sync.epoch==1709164800&&sync.offset==330);
    for(const auto& line:{"@TIME 0 0\n","@TIME 1709164800 841\n","@TIME 1709164800 330junk\n","@TIME -1 0\n","@TIME 999999999999999999 0\n"})assert(!feed(line));
    assert(!feed("@"+std::string(100,'s')+"\n"));
    assert(feed("@TIME 1709164800 -720\r\n"));assert(!console.consume('s',sync,ready));
    Preferences original;original.light=true;original.clock={true,false,DateFormat::YearMonthDay,-60};
    Preferences decoded;assert(decodePreferences(encodePreferences(original),decoded)&&decoded==original);
    std::array<std::uint8_t,8> legacy{{'R','1','5','P',1,1,0,0}};
    for(unsigned i=0;i<7;++i)legacy[7]^=legacy[i];
    assert(decodePreferences(legacy,decoded)&&decoded.light&&decoded.clock==ClockPreferences{});
    DisplayOptions view;ClusterState state;Lifecycle running;running.key(true,0);running.advance(2000);
    settingsAction(view,SettingsAction::Open,state,running);assert(view.settings);
    view.settingsFocus=3;settingsAction(view,SettingsAction::Select,state,running);assert(view.clock);
    settingsAction(view,SettingsAction::Select,state,running);assert(view.clockPreferences.twelveHour);
    view.clockFocus=6;assert(settingsAction(view,SettingsAction::Select,state,running)==SettingsEffect::SaveClockPreferences);
    clockSaveResult(view,false);assert(view.clock&&view.clockSaveFailed);
    settingsAction(view,SettingsAction::Back,state,running);assert(!view.clock&&!view.clockPreferences.twelveHour);
    return 0;
}
