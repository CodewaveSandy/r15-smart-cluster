#include "CrowPanel.h"
#include "BenchSweep.h"
#include "BenchTouch.h"
#include "PreferenceStore.h"
#include "VehicleUart.h"
#include "embedded/ui/Dashboard.h"
#include "embedded/application/DashboardInput.h"
#include "embedded/application/SettingsNavigation.h"
#include "embedded/presentation/ClockPresentation.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_random.h"
#include "driver/uart_vfs.h"
#include <algorithm>
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <fcntl.h>
#include <unistd.h>
#include "sdkconfig.h"
static_assert(CONFIG_ESP_MAIN_TASK_STACK_SIZE>=16384,
    "Display LVGL owner requires its existing 16384-byte main stack; update the generated display sdkconfig");

namespace {
constexpr char tag[]="r15-bench";
uint64_t nowMs() { return uint64_t(esp_timer_get_time()/1000); }
}
extern "C" void app_main() {
    ESP_LOGI(tag,"R15 UART INPUT BENCH; no speed, radio or ignition adapter");
    const auto psram=esp_psram_get_size();
    ESP_LOGI(tag,"IDF %s; PSRAM %u bytes",esp_get_idf_version(),unsigned(psram));
    if(psram<2*800*480*2) { ESP_LOGE(tag,"Insufficient PSRAM"); abort(); }
    cluster::board::initialize();
    // Only this task constructs, updates and renders LVGL objects.
    cluster::ui::Dashboard dashboard(lv_screen_active());
    cluster::ui::ViewOptions view;
    cluster::Clock wallClock;cluster::ClockConsole clockConsole;
    cluster::presentation::ClockText clockLabels;
    view.time=clockLabels.time;view.date=clockLabels.date;view.period=clockLabels.period;
    static cluster::board::PreferenceStore preferenceStore;
    cluster::Preferences confirmedPreferences;
    preferenceStore.initialize(confirmedPreferences);
    view.light=confirmedPreferences.light;view.clockPreferences=confirmedPreferences.clock;
    bool savePending=false,pendingClock=false;cluster::Preferences requestedPreferences;
    const auto savePreferences=[&](cluster::SettingsEffect effect) {
        if(savePending)return;
        pendingClock=effect==cluster::SettingsEffect::SaveClockPreferences;
        requestedPreferences=confirmedPreferences;
        if(pendingClock)requestedPreferences.clock=view.clockPreferences;
        else requestedPreferences.light=view.light;
        const auto complete=[&](bool success){if(pendingClock)cluster::clockSaveResult(view,success);else cluster::appearanceSaveResult(view,success);};
        if(requestedPreferences==confirmedPreferences){complete(true);return;}
        savePending=preferenceStore.request(requestedPreferences);
        if(pendingClock)view.clockSaving=savePending;else view.appearanceSaving=savePending;
        if(!savePending)complete(false);
    };
    // Return frames share console TX GPIO43 with USB bridge RX. Preserve all
    // binary bytes; only the external RX input (vehicle GPIO18) is attached.
    uart_vfs_dev_port_set_tx_line_endings(0,ESP_LINE_ENDINGS_LF);
    static cluster::board::VehicleUart vehicleUart;vehicleUart.initialize(esp_random());
    const auto dispatch=[&](cluster::SettingsEffect effect,std::uint64_t now){
        if(effect==cluster::SettingsEffect::SavePreferences||effect==cluster::SettingsEffect::SaveClockPreferences)savePreferences(effect);
        else if(effect!=cluster::SettingsEffect::None)vehicleUart.command(effect,now);
    };
    bool live=true;
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
    ESP_LOGI(tag,"Keys: l layout, t theme, d moving demo/unknown, w test warnings, n trip, i info, u units, b boot, o shutdown, v live UART, p parked fixture, s Settings, q Back, [/] focus, Enter select. Touch: center demo, top-left layout, top-right theme, bottom-left trip, bottom-middle info, fuel area shutdown; tap black screen to boot.");
    bool previousPairRequest=false,touchTap=false;
    int touchStartX=0,touchStartY=0;
    uint64_t report=nowMs(),clockRequest=0,formattedMinute=~uint64_t(0);
    cluster::ClockPreferences formattedPreferences;int formattedOffset=9999;
    unsigned previousFlushes=cluster::board::flushCount();std::uint64_t lastLoop=0,maxLoopGap=0,maxUiUs=0;
    for(;;) {
        const auto now=nowMs();
        if(lastLoop)maxLoopGap=std::max(maxLoopGap,now-lastLoop);
        lastLoop=now;
        if(!wallClock.available(now)&&now-clockRequest>=5000){std::printf("@CLOCK REQUEST\n");clockRequest=now;}
        vehicleUart.poll(now);
        if(live)vehicleUart.synchronizeClock(wallClock,now);
        const auto clicks=vehicleUart.takeClicks();
        if(live) {
            state=vehicleUart.state(now);
            for(unsigned i=0;i<clicks.trip;++i){cluster::applyAction(view,cluster::DashboardAction::NextTrip);}
            for(unsigned i=0;i<clicks.info;++i){cluster::applyAction(view,cluster::DashboardAction::NextInfo);}
        }
        if(live&&life.phase()==cluster::Phase::Running) {
            if(state.phone.available()&&state.phone.pairingRequested&&!previousPairRequest) {
                view.settings=true;cluster::cancelClock(view);cluster::cancelAppearance(view);view.phone=true;view.phoneFocus=0;view.phoneForgetConfirmation=false;
            }
            previousPairRequest=state.phone.available()&&state.phone.pairingRequested;
        }
        char command=0;
        cluster::board::Touch touch;
        if(cluster::board::readTouch(touch)) {
            if(touch.phase==cluster::board::TouchPhase::Press){touchTap=true;touchStartX=touch.x;touchStartY=touch.y;}
            if(touch.phase==cluster::board::TouchPhase::Cancel||std::abs(touch.x-touchStartX)>=6||std::abs(touch.y-touchStartY)>=6)touchTap=false;
            if(touch.phase==cluster::board::TouchPhase::Release&&touchTap){
                if(const auto effect=dashboard.touch(view,state,life,touch.x,touch.y))dispatch(*effect,now);
                else command=cluster::board::benchTouchAction(life.phase(),touch.x,touch.y);
                touchTap=false;
            }
        }
        char input[16];
        const auto length=std::max<ssize_t>(0,read(STDIN_FILENO,input,sizeof input));
        // Bounded console work. Serial and touch map to the same semantic actions.
        for(int j=-1;j<length;++j) {
            const char key=j<0?command:input[j];
            if(j>=0) {
                cluster::ClockSync sync;bool ready=false;
                if(clockConsole.consume(key,sync,ready)) {
                    if(ready&&wallClock.synchronize(sync.epoch,sync.offset,now))std::printf("@CLOCK OK %llu\n",static_cast<unsigned long long>(sync.epoch));
                    continue;
                }
            }
            if(shutdownPreview && key!='b') continue;
            using cluster::DashboardAction;
            switch(key) {
            case 'v': live=true;demo=warnings=false;view.iconTest=false;vehicleUart.reset();state={};break;
            case 'p': live=false; demo=false; warnings=false; state=cluster::board::parkedSettingsFixture(); view.iconTest=false; break;
            case 's': dispatch(cluster::settingsAction(view,cluster::SettingsAction::Open,state,life),now); break;
            case 'q': dispatch(cluster::settingsAction(view,cluster::SettingsAction::Back,state,life),now); break;
            case ']': cluster::settingsAction(view,cluster::SettingsAction::Next,state,life); break;
            case '[': cluster::settingsAction(view,cluster::SettingsAction::Previous,state,life); break;
            case '\n': case '\r': {
                const auto effect=cluster::settingsAction(view,cluster::SettingsAction::Select,state,life);
                dispatch(effect,now);
                break; }
            case 'l': cluster::applyAction(view,DashboardAction::ToggleLayout); break;
            case 't': cluster::applyAction(view,DashboardAction::ToggleTheme); break;
            case 'n': cluster::applyAction(view,DashboardAction::NextTrip); break;
            case 'i': cluster::applyAction(view,DashboardAction::NextInfo); break;
            case 'u': cluster::applyAction(view,DashboardAction::ToggleUnits); break;
            case 'd': demo=!demo; live=!demo; demoStarted=now; state={}; warnings=false; break;
            case 'w': live=false;warnings=!warnings;
                if(!warnings && !demo) state={};
                break;
            case 'b': live=true;vehicleUart.reset();shutdownPreview=false; demo=false; warnings=false; state={}; view.iconTest=false;
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
        view.simulated=!live;
        view.vehicleLink=vehicleUart.fresh(now)?cluster::ui::VehicleLink::Connected:
            vehicleUart.received()?cluster::ui::VehicleLink::Stale:cluster::ui::VehicleLink::Waiting;
        if(wallClock.available(now))view.clockSourceOffset=wallClock.offset();
        const auto epoch=wallClock.epoch(now);
        if(epoch/60!=formattedMinute||!(view.clockPreferences==formattedPreferences)||wallClock.offset()!=formattedOffset) {
            clockLabels=cluster::presentation::clockText(epoch,wallClock.offset(),view.clockPreferences);
            formattedMinute=epoch/60;formattedPreferences=view.clockPreferences;formattedOffset=wallClock.offset();
        }
        bool saved=false;
        if(savePending&&preferenceStore.result(saved)) {
            savePending=false;
            if(saved) confirmedPreferences=requestedPreferences;
            if(pendingClock){if(view.clock&&view.clockSaving)cluster::clockSaveResult(view,saved);}
            else if(view.appearance&&view.appearanceSaving)cluster::appearanceSaveResult(view,saved);
            ESP_LOGI(tag,"Preferences save %s",saved?"committed":"failed");
        }
        life.advance(now); dispatch(cluster::reconcileSettings(view,state,life),now);
        const auto uiStarted=esp_timer_get_time();
        dashboard.guidance(live?vehicleUart.guidanceState(nowMs()):cluster::guidance::TurnGuidanceState{});
        dashboard.update(state,view,life,now);
        lv_timer_handler();
        if(live)vehicleUart.guidancePublished(nowMs(),dashboard.guidanceVisible());
        maxUiUs=std::max(maxUiUs,std::uint64_t(esp_timer_get_time()-uiStarted));
        if(now-report>=10000) {
            const auto tx=vehicleUart.txDiagnostics();const auto g=vehicleUart.guidanceDiagnostics();const auto f=vehicleUart.framingDiagnostics();const auto guide=vehicleUart.guidanceState(now);
            ESP_LOGI(tag,"GUIDANCE accepted=%u rejected=%u duplicate=%u expired=%u quality=%u source_age_ms=%u tx_fragments=%u framed_bytes=%u rx_fragments=%u crc_drops=%u framing_drops=%u timeouts=%u",g.accepted,g.rejected,g.duplicates,g.expired,unsigned(guide.quality),guide.ageMs,tx.fragments,tx.framedBytes,f.fragments,f.crcDrops,f.framingDrops,f.timeouts);
            ESP_LOGI(tag,"UART TX bytes=%u errors=%u queue_full=%u stack_margin=%u critical_wait_ms=%u ui_max_us=%llu loop_gap_ms=%llu touch_errors=%u",tx.bytes,tx.writeErrors,vehicleUart.txRejected(),vehicleUart.txStackMargin(),tx.maxCriticalWaitMs,(unsigned long long)maxUiUs,(unsigned long long)maxLoopGap,cluster::board::touchFailures());
            ESP_LOGI(tag,"UART RX GPIO38: accepted=%u overruns/errors=%u fresh=%d source=%s rx_bytes=%u max_backlog=%u",
                vehicleUart.accepted(),vehicleUart.overruns(),int(vehicleUart.fresh(now)),live?"UART":"BENCH",vehicleUart.rxBytes(),vehicleUart.maxBacklog());
            const auto linkPhone=vehicleUart.state(now).phone;
            ESP_LOGI(tag,"PHONE_LINK quality=%u unavailable=%u status_packets=%u status_age_ms=%llu max_status_gap_ms=%llu",
                unsigned(linkPhone.quality),unsigned(linkPhone.unavailable),vehicleUart.statusAccepted(),
                (unsigned long long)vehicleUart.statusAge(now),(unsigned long long)vehicleUart.maxStatusGap());
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
