#include "CrowPanel.h"
#include "embedded/ui/Dashboard.h"
#include "embedded/application/DashboardInput.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include <algorithm>
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <fcntl.h>
#include <unistd.h>

namespace {
constexpr char tag[]="r15-bench";
cluster::ClusterState synthetic() {
    using cluster::Quality;
    cluster::ClusterState s;
    s.speedKph={60,Quality::Available}; s.engineRpm={8500,Quality::Available};
    s.gear={4,Quality::Available}; s.fuelPercent={72,Quality::Available};
    s.currentLapMs={84380,Quality::Available}; s.bestLapMs={78420,Quality::Available};
    s.trip1Km={128.4,Quality::Available}; s.trip2Km={46.8,Quality::Available};
    s.coolantTemperatureC={84,Quality::Available};
    // Real odometer/clock/phone/warnings remain unknown. No persistence.
    return s;
}
uint64_t nowMs() { return uint64_t(esp_timer_get_time()/1000); }
}
extern "C" void app_main() {
    ESP_LOGI(tag,"R15 DISPLAY BENCH ONLY; no vehicle, radio, ignition or storage adapter");
    const auto psram=esp_psram_get_size();
    ESP_LOGI(tag,"IDF %s; PSRAM %u bytes",esp_get_idf_version(),unsigned(psram));
    if(psram<2*800*480*2) { ESP_LOGE(tag,"Insufficient PSRAM"); abort(); }
    cluster::board::initialize();
    // Only this task constructs, updates and renders LVGL objects.
    cluster::ui::Dashboard dashboard(lv_screen_active());
    cluster::ui::ViewOptions view;
    view.date="BENCH / NO VEHICLE";
    cluster::ClusterState state;
    cluster::Lifecycle life;
    bool demo=false, warnings=false;
    // Establish a visible black first frame before starting the artwork clock.
    dashboard.update(state,view,life,nowMs()); lv_refr_now(nullptr);
    cluster::board::backlight(true);
    life.key(true,nowMs());
    const int flags=fcntl(STDIN_FILENO,F_GETFL,0);
    if(flags>=0) fcntl(STDIN_FILENO,F_SETFL,flags|O_NONBLOCK);
    ESP_LOGI(tag,"Keys: l layout, t theme, d synthetic/unknown, w test warnings, n trip, i info, u units, b boot, o shutdown. Touch: top-left layout, top-right theme, bottom-left trip, bottom-middle info.");
    int x=0,y=0;
    uint64_t report=nowMs();
    for(;;) {
        const auto now=nowMs();
        char command=0;
        if(cluster::board::releasedTouch(x,y) && life.phase()==cluster::Phase::Running) {
            if(y<85) command=x<400?'l':'t';
            else if(y>=376 && x<300) command='n';
            else if(y>=376 && x<580) command='i';
        }
        char input[16];
        const auto length=std::max<ssize_t>(0,read(STDIN_FILENO,input,sizeof input));
        // Bounded console work. Serial and touch map to the same semantic actions.
        for(int j=-1;j<length;++j) {
            const char key=j<0?command:input[j];
            using cluster::DashboardAction;
            switch(key) {
            case 'l': cluster::applyAction(view,DashboardAction::ToggleLayout); break;
            case 't': cluster::applyAction(view,DashboardAction::ToggleTheme); break;
            case 'n': cluster::applyAction(view,DashboardAction::NextTrip); break;
            case 'i': cluster::applyAction(view,DashboardAction::NextInfo); break;
            case 'u': cluster::applyAction(view,DashboardAction::ToggleUnits); break;
            case 'd': demo=!demo; state=demo?synthetic():cluster::ClusterState{}; warnings=false;
                view.date=demo?"BENCH / SYNTHETIC DATA":"BENCH / NO VEHICLE"; break;
            case 'w': warnings=!warnings;
                state.engineWarning=state.coolantWarning=state.absWarning=state.left=state.right=state.highBeam={warnings,cluster::Quality::Available};
                view.date="BENCH / TEST WARNINGS"; break;
            case 'b': life=cluster::Lifecycle{}; life.key(true,now); break;
            case 'o': life.key(false,now); break;
            default: break;
            }
        }
        life.advance(now); dashboard.update(state,view,life,now);
        lv_timer_handler();
        if(now-report>=10000) {
            report=now;
            ESP_LOGI(tag,"heap internal free/min/largest=%u/%u/%u; PSRAM free=%u; stack margin=%u; flushes=%u max_us=%u",
                unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
                unsigned(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)),
                unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)),
                unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
                unsigned(uxTaskGetStackHighWaterMark(nullptr)),cluster::board::flushCount(),cluster::board::maxFlushUs());
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
