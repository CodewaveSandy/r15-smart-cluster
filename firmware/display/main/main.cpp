#include "CrowPanel.h"
#include "BenchSweep.h"
#include "BenchTouch.h"
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
    bool shutdownPreview=false;
    uint64_t demoStarted=0;
    // Establish a visible black first frame before starting the artwork clock.
    dashboard.update(state,view,life,nowMs()); lv_refr_now(nullptr);
    cluster::board::backlight(true);
    life.key(true,nowMs());
    const int flags=fcntl(STDIN_FILENO,F_GETFL,0);
    if(flags>=0) fcntl(STDIN_FILENO,F_SETFL,flags|O_NONBLOCK);
    ESP_LOGI(tag,"Keys: l layout, t theme, d moving demo/unknown, w test warnings, n trip, i info, u units, b boot, o shutdown. Touch: center demo, top-left layout, top-right theme, bottom-left trip, bottom-middle info, fuel area shutdown; tap black screen to boot.");
    int x=0,y=0;
    uint64_t report=nowMs();
    unsigned previousFlushes=cluster::board::flushCount();
    for(;;) {
        const auto now=nowMs();
        char command=0;
        if(cluster::board::releasedTouch(x,y)) command=cluster::board::benchTouchAction(life.phase(),x,y);
        char input[16];
        const auto length=std::max<ssize_t>(0,read(STDIN_FILENO,input,sizeof input));
        // Bounded console work. Serial and touch map to the same semantic actions.
        for(int j=-1;j<length;++j) {
            const char key=j<0?command:input[j];
            if(shutdownPreview && key!='b') continue;
            using cluster::DashboardAction;
            switch(key) {
            case 'l': cluster::applyAction(view,DashboardAction::ToggleLayout); break;
            case 't': cluster::applyAction(view,DashboardAction::ToggleTheme); break;
            case 'n': cluster::applyAction(view,DashboardAction::NextTrip); break;
            case 'i': cluster::applyAction(view,DashboardAction::NextInfo); break;
            case 'u': cluster::applyAction(view,DashboardAction::ToggleUnits); break;
            case 'd': demo=!demo; demoStarted=now; state={}; warnings=false; break;
            case 'w': warnings=!warnings;
                if(!warnings && !demo) state={};
                break;
            case 'b': shutdownPreview=false; demo=false; warnings=false; state={}; view.iconTest=false;
                life=cluster::Lifecycle{}; life.key(true,now); break;
            case 'o': shutdownPreview=true; warnings=false; view.iconTest=false;
                // Stop synthetic urgent warnings from interrupting this explicit
                // artwork preview; the shared real-warning priority is unchanged.
                state.engineWarning=state.coolantWarning={false,cluster::Quality::Available};
                life.key(false,now); break;
            default: break;
            }
        }
        if(demo && !shutdownPreview) state=cluster::board::benchSweep(now-demoStarted);
        if(warnings) state.engineWarning=state.coolantWarning=state.absWarning=state.left=state.right=state.highBeam={true,cluster::Quality::Available};
        view.iconTest=demo&&!shutdownPreview&&cluster::board::benchLampTestLit(now-demoStarted);
        view.date=shutdownPreview?"BENCH / SHUTDOWN TEST":warnings?"BENCH / TEST WARNINGS":
            demo&&cluster::board::benchLampTest(now-demoStarted)?"BENCH / LAMP TEST":demo?"BENCH / SYNTHETIC DATA":"BENCH / NO VEHICLE";
        life.advance(now); dashboard.update(state,view,life,now);
        lv_timer_handler();
        if(now-report>=10000) {
            const unsigned frames=cluster::board::flushCount();
            ESP_LOGI(tag,"rendered updates/s=%.1f (10s average; includes idle time)",
                double(frames-previousFlushes)*1000/double(now-report));
            previousFlushes=frames;
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
