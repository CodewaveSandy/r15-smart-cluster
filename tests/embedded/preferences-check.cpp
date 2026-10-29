#include "simulator/lvgl/PreferenceFile.h"
#include <chrono>
#include "embedded/application/SettingsNavigation.h"
#include <cstdio>
int main(int argc,char** argv) {
    using namespace cluster;
    int failed=0;
    const auto check=[&](bool ok,const char* why){if(!ok){++failed;std::fprintf(stderr,"FAIL: %s\n",why);}};
    {
        DisplayOptions options; ClusterState state; state.speedKph={0,Quality::Available};
        Lifecycle life;life.key(true,0);life.advance(Lifecycle::bootMs);
        options.settings=options.appearance=options.appearanceSaving=true;
        options.appearanceFocus=1;
        check(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::None&&!options.light,"Pending commit blocks another theme selection");
        options.appearanceFocus=4;
        check(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::None,"Pending commit suppresses repeated Apply");
        settingsAction(options,SettingsAction::Back,state,life);
        check(!options.appearance&&!options.appearanceSaving,"Back cancels preview while storage completes independently");
    }
    Preferences value;
    const auto valid=encodePreferences({true,DashboardDesign::Instrument});
    check(decodePreferences(valid,value)&&value.light,"Preference codec round trip");
    for(unsigned field:{0u,4u,5u,6u,7u,8u,9u,12u,15u}) {
        auto corrupt=valid; corrupt[field]^=field==8?3:2;
        if(field!=15) { corrupt[15]=0; for(unsigned i=0;i<15;++i) corrupt[15]^=corrupt[i]; }
        check(!decodePreferences(corrupt,value),"Reject invalid magic/version/theme/design/checksum");
    }
    if(argc!=2) return 1;
    const auto root=std::filesystem::path(argv[1])/("preferences-check-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto path=root/"preferences.bin";
    host::PreferenceFile file(path);
    value.light=true;
    check(!file.load(value)&&!value.light,"Missing file uses defaults");
    {
        std::filesystem::create_directories(root);
        std::array<std::uint8_t,8> legacy{{'R','1','5','P',1,1,0,0}};
        for(unsigned i=0;i<7;++i)legacy[7]^=legacy[i];
        std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(legacy.data()),legacy.size());output.close();
        check(file.load(value)&&value.light&&value.clock==ClockPreferences{},"Legacy preference file retains theme and defaults clock");
    }
    Preferences clockSaved;clockSaved.light=true;clockSaved.clock={true,false,DateFormat::YearMonthDay,-240};
    check(file.save(clockSaved)&&file.load(value)&&value==clockSaved,"Clock formats/timezone round trip with theme");
    check(file.save({true,DashboardDesign::Instrument}),"Save confirmed Daylight");
    host::PreferenceFile restarted(path);
    check(restarted.load(value)&&value.light,"Restart restores theme");
    check(file.save({false,DashboardDesign::Instrument})&&restarted.load(value)&&!value.light,"Atomic replacement restores Midnight");
    auto blockedTemporary=path; blockedTemporary+=".tmp";
    std::filesystem::create_directory(blockedTemporary);
    check(!file.save({true,DashboardDesign::Instrument})&&restarted.load(value)&&!value.light,"Failed replacement preserves confirmed file");
    std::filesystem::remove(blockedTemporary);
    for(unsigned length:{0u,3u,9u}) {
        std::ofstream output(path,std::ios::binary|std::ios::trunc);
        const std::array<char,9> data{}; output.write(data.data(),length); output.close();
        value.light=true;
        check(!file.load(value)&&!value.light,"Malformed size uses defaults");
    }
    {
        std::ofstream output(root/"blocked"); output<<"file, not a directory";
    }
    check(!host::PreferenceFile(root/"blocked/preferences.bin").save({true,DashboardDesign::Instrument}),"Storage failure is reported");
    check(!file.save({true,static_cast<DashboardDesign>(99)}),"Invalid design is never saved");
    std::filesystem::remove_all(root);
    return failed?1:0;
}
