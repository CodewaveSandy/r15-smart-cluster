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
    for(unsigned field:{0u,4u,5u,6u,7u}) {
        auto corrupt=valid; corrupt[field]^=2;
        if(field!=7) { corrupt[7]=0; for(unsigned i=0;i<7;++i) corrupt[7]^=corrupt[i]; }
        check(!decodePreferences(corrupt,value),"Reject invalid magic/version/theme/design/checksum");
    }
    if(argc!=2) return 1;
    const auto root=std::filesystem::path(argv[1])/("preferences-check-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto path=root/"preferences.bin";
    host::PreferenceFile file(path);
    value.light=true;
    check(!file.load(value)&&!value.light,"Missing file uses defaults");
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
