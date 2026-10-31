#include "embedded/ui/Geometry.h"
#include "embedded/ui/Dashboard.h"
#include "embedded/ui/Typography.h"
#include <SDL.h>
#include "DesktopInput.h"
#include "PreferenceFile.h"
#include "BluetoothDemo.h"
#include "WifiDemo.h"
#include "PhoneBridge.h"
#include "VehicleSerial.h"
#include "embedded/presentation/ClockPresentation.h"
#include <cstdlib>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <ctime>
#include <vector>
#include <fstream>

namespace {
using namespace cluster;
std::array<std::uint16_t,800*480> pixels{};
int warnings=0;
struct FlushMetrics { std::uint64_t calls=0, pixels=0; };
FlushMetrics flushMetrics;
bool framebufferDirty=true;
void logMessage(lv_log_level_t, const char* message) { ++warnings; std::fprintf(stderr,"%s",message); }
void flush(lv_display_t* display,const lv_area_t* area,std::uint8_t* bytes) {
    framebufferDirty=true;
    ++flushMetrics.calls;
    flushMetrics.pixels+=std::uint64_t(area->x2-area->x1+1)*(area->y2-area->y1+1);
    const auto* source=reinterpret_cast<std::uint16_t*>(bytes);
    for(int y=area->y1;y<=area->y2;++y)
        std::copy_n(source+(y-area->y1)*(area->x2-area->x1+1),area->x2-area->x1+1,pixels.data()+y*800+area->x1);
    lv_display_flush_ready(display);
}
struct PresentWork { bool uploaded=false, presented=false; };
PresentWork presentFramebuffer(SDL_Renderer* renderer,SDL_Texture* texture,bool& repaint) {
    PresentWork work;
    if(framebufferDirty) {
        if(SDL_UpdateTexture(texture,nullptr,pixels.data(),800*2))
            throw std::runtime_error(SDL_GetError());
        framebufferDirty=false;
        repaint=true;
        work.uploaded=true;
    }
    if(repaint) {
        if(SDL_RenderClear(renderer) || SDL_RenderCopy(renderer,texture,nullptr,nullptr))
            throw std::runtime_error(SDL_GetError());
        SDL_RenderPresent(renderer);
        repaint=false;
        work.presented=true;
    }
    return work;
}
// Exercise the same presentation path as the interactive preview under dummy SDL.
void checkPresentation() {
    auto* surface=SDL_CreateRGBSurfaceWithFormat(0,800,480,16,SDL_PIXELFORMAT_RGB565);
    if(!surface) throw std::runtime_error(SDL_GetError());
    std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> ownedSurface(surface,SDL_FreeSurface);
    std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)> renderer(SDL_CreateSoftwareRenderer(surface),SDL_DestroyRenderer);
    if(!renderer) throw std::runtime_error(SDL_GetError());
    std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> texture(SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,800,480),SDL_DestroyTexture);
    if(!texture) throw std::runtime_error(SDL_GetError());
    bool repaint=true;
    framebufferDirty=true;
    auto work=presentFramebuffer(renderer.get(),texture.get(),repaint);
    if(!work.uploaded || !work.presented) throw std::runtime_error("Initial framebuffer was not presented");
    for(int i=0;i<100;++i) {
        work=presentFramebuffer(renderer.get(),texture.get(),repaint);
        if(work.uploaded || work.presented) throw std::runtime_error("Idle SDL presentation repeated");
    }
    repaint=true;
    work=presentFramebuffer(renderer.get(),texture.get(),repaint);
    if(work.uploaded || !work.presented) throw std::runtime_error("Window repaint must reuse texture");
    framebufferDirty=true;
    work=presentFramebuffer(renderer.get(),texture.get(),repaint);
    if(!work.uploaded || !work.presented) throw std::runtime_error("Changed framebuffer was not presented");
}
ClusterState demo() {
    ClusterState s;
    cluster::host::WifiDemo::populate(s.wifi);
    s.bluetooth.quality=Quality::Available; s.bluetooth.paired=true; s.bluetooth.connected=true;
    std::snprintf(s.bluetooth.deviceName.data(),s.bluetooth.deviceName.size(),"Rider phone");
    s.bestLapMs={78420,Quality::Available}; // Static best lap sample: 01:18.42.
    s.currentLapMs={84380,Quality::Available}; // Static lap sample: 01:24.38.
    s.speedKph={60,Quality::Available}; s.engineRpm={8500,Quality::Available};
    s.quickShifter={true,Quality::Available}; s.vva={true,Quality::Available};
    s.phoneConnected={true,Quality::Available}; s.phoneBatteryPercent={84,Quality::Available};
    s.gear={4,Quality::Available}; s.fuelPercent={72,Quality::Available};
    s.odometerKm={12480,Quality::Available};
    // Static host preview samples, not computed or persisted vehicle distances.
    s.trip1Km={128.4,Quality::Available}; s.trip2Km={46.8,Quality::Available};
    s.tcsEnabled={true,Quality::Available}; s.batteryVoltageV={12.8,Quality::Available};
    s.coolantTemperatureC={84,Quality::Available};
    // Explicit host samples; vehicle MCU will own measurement/averaging windows.
    s.realtimeFuelLPer100Km={3.4,Quality::Available}; s.averageFuelLPer100Km={2.8,Quality::Available};
    s.averageSpeedKph={47.2,Quality::Available};
    s.left=s.right=s.highBeam=s.absWarning=s.engineWarning=s.coolantWarning={false,Quality::Available};
    return s;
}
void capture(const std::filesystem::path& path) {
    lv_refr_now(nullptr);
    auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),800,480,16,800*2,SDL_PIXELFORMAT_RGB565);
    if(!surface) throw std::runtime_error(SDL_GetError());
    const int result=SDL_SaveBMP(surface,path.string().c_str()); SDL_FreeSurface(surface);
    if(result) throw std::runtime_error(SDL_GetError());
}
void help() {
    std::puts("R15 LVGL preview — simulated data only\n"
        "Left/Right: speed +/-1 | Up/Down: RPM +/-100 | [ / ]: fuel +/-1\n"
        "0..6: gear (0 neutral) | Tab: Street/Track | D: Midnight/Daylight\n"
        "Enter: Street trip page | I: info page | T: toggle selected TCS/Track (simulation only)\n"
        "U: metric/imperial | X: unavailable/restore\n"
        "L/R: turn signals | Z: hazards | H: high beam | A/E/C: warnings\n"
        "P: stationary demo | S: settings | Arrows: focus | Enter: select | Backspace/Esc: back\n"
        "O: key on/off (2000 ms boot / 2000 ms shutdown) | Space: reset demo\n"
        "F5/F6: Wi-Fi fail next request / unavailable toggle\n"
        "F3: fail next Bluetooth request | F4: Bluetooth service unavailable/restore\n"
        "F1: print controls | F2: all icons | Esc: quit | No real bike connected.");
}
}
int main(int argc,char** argv) {
    try {
        bool render=false,animationSequence=false; std::filesystem::path output;
        const char* vehiclePort=nullptr;
        if(argc==3&&(std::string(argv[1])=="--render"||std::string(argv[1])=="--animate")) { render=true; animationSequence=std::string(argv[1])=="--animate"; output=argv[2]; }
        else if(argc==3&&std::string(argv[1])=="--vehicle-serial") vehiclePort=argv[2];
        else if(argc!=1) { std::fprintf(stderr,"Usage: r15-preview [--render directory | --animate directory | --vehicle-serial port]\n"); return 2; }
        cluster::host::PreferenceFile preferenceFile(render?std::filesystem::path{}:cluster::host::preferencePath());
        Preferences confirmedPreferences;
        if(!render) preferenceFile.load(confirmedPreferences);
        if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)) throw std::runtime_error(SDL_GetError());
        struct SdlExit { ~SdlExit(){SDL_Quit();} } sdlExit;
        lv_init();
        lv_log_register_print_cb(logMessage);
        auto* display=lv_display_create(800,480);
        lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
        std::vector<std::uint16_t> drawBuffer(800*40);
        lv_display_set_buffers(display,drawBuffer.data(),nullptr,drawBuffer.size()*2,LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(display,flush);
        {
            cluster::ui::Dashboard dashboard(lv_screen_active());
            ClusterState state=demo(); cluster::ui::ViewOptions view; Lifecycle life;
            view.light=confirmedPreferences.light; view.clockPreferences=confirmedPreferences.clock; view.bluetoothDemo=true;view.simulated=true;
            if(render) {
                checkPresentation();
                std::filesystem::create_directories(output);
                view.time="20:55"; view.date="18 Oct 2026";
                life.key(true,0); life.advance(Lifecycle::bootMs);
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    const std::string suffix=std::string(track?"track":"street")+(light?"-day":"-night");
                    dashboard.update(state,view,life,1400); capture(output/(suffix+".bmp"));
                    const auto palette=light?cluster::ui::daylight:cluster::ui::midnight;
                    if(pixels[cluster::ui::Metrics::lowerDividerY*800+300]!=lv_color_to_u16(lv_color_hex(palette.line)) ||
                        pixels[(cluster::ui::Metrics::rpmY+cluster::ui::Metrics::rpmHeight)*800+300]!=lv_color_to_u16(lv_color_hex(palette.background)))
                        throw std::runtime_error("Layout vertical geometry differs from shared metrics");
                    auto low=state; low.fuelPercent.value=9;
                    dashboard.update(low,view,life,0); capture(output/(suffix+"-fuel-on.bmp"));
                    const auto fuelOn=pixels[cluster::ui::Metrics::fuelY*800+590];
                    dashboard.update(low,view,life,500); capture(output/(suffix+"-fuel-off.bmp"));
                    if(fuelOn==pixels[cluster::ui::Metrics::fuelY*800+590]) throw std::runtime_error("Fuel blink did not reach rendered pixels");
                    for(int percent:{0,6,16,17,56,100}) {
                        auto partial=state; partial.fuelPercent.value=percent;
                        dashboard.update(partial,view,life,0);
                        capture(output/(suffix+"-fuel-"+std::to_string(percent)+".bmp"));
                        if(percent==56) {
                            const auto segment=cluster::ui::geometry::FuelLayout::segment(4);
                            int filled=0,empty=0;
                            for(int x=segment.x;x<segment.x+segment.width;++x) {
                                const auto pixel=pixels[(segment.y+segment.height/2)*800+x];
                                if(pixel==lv_color_to_u16(lv_color_hex(palette.accent))) ++filled;
                                if(pixel==lv_color_to_u16(lv_color_hex(palette.panel))) ++empty;
                            }
                            if(!filled||!empty) throw std::runtime_error("Partial fuel segment lost its filled or empty portion");
                        }
                    }
                    ClusterState missing; dashboard.update(missing,view,life,1400); capture(output/(suffix+"-unavailable.bmp"));
                    const auto oldDate=view.date;const auto oldLink=view.vehicleLink;view.vehicleLink=cluster::ui::VehicleLink::Connected;
                    for(int gear:{0,1,2,3,4,5,6}) {
                        ClusterState usb;usb.gear={gear,Quality::Available};
                        usb.turnSignalsArePhase=true;usb.left=usb.right=usb.highBeam={true,Quality::Available};
                        dashboard.update(usb,view,life,1400);capture(output/(suffix+"-usb-gear-"+std::to_string(gear)+".bmp"));
                    }
                    ClusterState conflict;conflict.gear.quality=Quality::Invalid;
                    dashboard.update(conflict,view,life,1400);capture(output/(suffix+"-usb-conflict.bmp"));
                    conflict.gear.quality=Quality::Stale;view.vehicleLink=cluster::ui::VehicleLink::Stale;
                    dashboard.update(conflict,view,life,1400);capture(output/(suffix+"-usb-stale.bmp"));
                    for(const char* status:{"WAITING","LIVE","STALE"}) {
                        view.vehicleLink=std::strcmp(status,"LIVE")==0?cluster::ui::VehicleLink::Connected:std::strcmp(status,"STALE")==0?cluster::ui::VehicleLink::Stale:cluster::ui::VehicleLink::Waiting;
                        ClusterState uart;uart.turnSignalsArePhase=true;
                        if(std::strcmp(status,"LIVE")==0) {
                            uart.gear={2,Quality::Available};uart.left={true,Quality::Available};
                            uart.right=uart.highBeam={false,Quality::Available};
                        } else if(std::strcmp(status,"STALE")==0)
                            uart.gear.quality=uart.left.quality=uart.right.quality=uart.highBeam.quality=Quality::Stale;
                        dashboard.update(uart,view,life,1400);capture(output/(suffix+"-uart-"+status+".bmp"));
                    }
                    view.date=oldDate;view.vehicleLink=oldLink;
                    auto warning=state; warning.engineRpm.value=11400; warning.engineWarning.value=true; warning.absWarning.value=true;
                    dashboard.update(warning,view,life,1200); capture(output/(suffix+"-warning.bmp"));
                }
                Lifecycle boot; boot.key(true,0); dashboard.update(state,view,boot,625); capture(output/"boot.bmp");
                boot.key(false,700); dashboard.update(state,view,boot,1325); capture(output/"shutdown.bmp");
                auto urgentState=state; urgentState.engineWarning.value=true;
                dashboard.update(urgentState,view,boot,1325); capture(output/"shutdown-warning.bmp");
                Lifecycle off; dashboard.update(ClusterState{},view,off,0); capture(output/"off.bmp");
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light; view.iconTest=true;
                    const auto name=std::string("icons-")+(track?"track":"street")+(light?"-day.bmp":"-night.bmp");
                    dashboard.update(state,view,life,1400); capture(output/name);
                }
                view.iconTest=false;
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    view.tripPage=TripPage::Odometer; view.infoPage=InfoPage::Coolant;
                    std::vector<std::uint16_t> bestPanel;
                    for(int page=0;page<int(InfoPage::Count);++page) {
                        auto sample=state;
                        if(page==5) sample.tcsEnabled.value=false;
                        dashboard.update(sample,view,life,1400);
                        const auto name=std::string(track?"track":"street")+(light?"-day":"-night")+"-pages-"+std::to_string(page)+".bmp";
                        capture(output/name);
                        if(track) {
                            std::vector<std::uint16_t> panel;
                            for(int y=399;y<460;++y)
                                panel.insert(panel.end(),pixels.begin()+y*800+28,pixels.begin()+y*800+248);
                            if(page==0) bestPanel=panel;
                            else if(panel!=bestPanel) throw std::runtime_error("Track best-lap panel changed with Street trip selection");
                        }
                        view.tripPage=next(view.tripPage); view.infoPage=next(view.infoPage);
                    }
                }
                // Include dim placeholder ink when checking the centered three-position speed group.
                view.iconTest=false;
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    const auto palette=light?cluster::ui::daylight:cluster::ui::midnight;
                    for(int value:{-1,0,1,9,10,60,99,100,180,400}) {
                        auto sample=state;
                        sample.speedKph=value<0?Signal<int>{}:Signal<int>{value,Quality::Available};
                        if(track) sample.currentLapMs=value<0?Signal<std::uint32_t>{}:Signal<std::uint32_t>{static_cast<std::uint32_t>(value)*1000,Quality::Available};
                        dashboard.update(sample,view,life,1400); lv_refr_now(nullptr);
                        // Units now intentionally share the instrument ink.
                        // Exclude their object rectangle from digit-only bounds.
                        const auto findUnit=[&](auto&& self,lv_obj_t* obj)->lv_obj_t* {
                            if(lv_obj_check_type(obj,&lv_label_class)&&lv_obj_is_visible(obj)&&
                               lv_obj_get_style_text_font(obj,0)==cluster::ui::fonts::unit) return obj;
                            for(uint32_t i=0;i<lv_obj_get_child_count(obj);++i)
                                if(auto* found=self(self,lv_obj_get_child(obj,i))) return found;
                            return nullptr;
                        };
                        auto* unit=findUnit(findUnit,lv_screen_active());
                        if(!unit) throw std::runtime_error("Visible speed unit missing");
                        lv_area_t unitBounds; lv_obj_get_coords(unit,&unitBounds);
                        const auto background=lv_color_to_u16(lv_color_hex(palette.background));
                        const auto numericInk=[&](int x,int y) {
                            const bool inUnit=x>=unitBounds.x1&&x<=unitBounds.x2&&y>=unitBounds.y1&&y<=unitBounds.y2;
                            return !inUnit&&pixels[y*800+x]!=background;
                        };
                        int left=800,right=-1;
                        const auto* font=track?cluster::ui::fonts::track:cluster::ui::fonts::speed;
                        const int primaryY=track?cluster::ui::Metrics::trackLapY:240;
                        for(int y=primaryY;y<primaryY+font->line_height;++y)
                            for(int x=cluster::ui::Metrics::speedX;x<cluster::ui::Metrics::speedX+cluster::ui::Metrics::speedWidth;++x)
                                if(numericInk(x,y)) { left=std::min(left,x); right=std::max(right,x); }
                        if(right<left||std::abs(left+right-799)>4) throw std::runtime_error("Primary instrument glyphs are not centered on the viewport: value="+std::to_string(value)+" light="+std::to_string(light)+" left="+std::to_string(left)+" right="+std::to_string(right));
                        if(track) {
                            left=800; right=-1;
                            for(int y=269;y<269+cluster::ui::fonts::secondary->line_height;++y)
                                for(int x=560;x<772;++x)
                                    if(numericInk(x,y)) { left=std::min(left,x); right=std::max(right,x); }
                            if(right<left||std::abs(left+right-1331)>4) throw std::runtime_error("Track speed is not centered in its secondary position: value="+std::to_string(value)+" left="+std::to_string(left)+" right="+std::to_string(right));
                        }
                    }
                }
                // Host work counters, not ESP32 timing or memory measurements.
                view.track=false; view.light=false;
                dashboard.update(state,view,life,1400); lv_refr_now(nullptr);
                const auto stablePixels=pixels;
                flushMetrics={};
                const auto measuredStart=std::chrono::steady_clock::now();
                for(int i=0;i<100;++i) {
                    dashboard.update(state,view,life,1400);
                    lv_refr_now(nullptr);
                }
                if(flushMetrics.calls!=0) throw std::runtime_error("Identical dashboard updates caused redundant flushes");
                const auto idleMetrics=flushMetrics;
                const auto elapsedUs=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-measuredStart).count();
                if(pixels!=stablePixels) throw std::runtime_error("Repeated identical presentation changed pixels");
                const auto countObjects=[](auto&& self,lv_obj_t* obj)->std::uint32_t {
                    std::uint32_t count=1;
                    for(std::uint32_t i=0;i<lv_obj_get_child_count(obj);++i) count+=self(self,lv_obj_get_child(obj,i));
                    return count;
                };
                std::ofstream report(output/"host-metrics.json");
                report << "{\n  \"scope\": \"host only; not firmware resource or timing evidence\",\n"
                    << "  \"scenario\": \"100 identical Street/Midnight updates at fixed monotonic time\",\n"
                    << "  \"iterations\": 100,\n  \"flush_calls\": " << idleMetrics.calls
                    << ",\n  \"flushed_pixels\": " << idleMetrics.pixels
                    << ",\n  \"elapsed_us\": " << elapsedUs
                    << ",\n  \"lvgl_object_count_including_screen\": " << countObjects(countObjects,lv_screen_active())
                    << ",\n  \"dashboard_wrapper_bytes_on_host\": " << sizeof(cluster::ui::Dashboard)
                    << ",\n  \"host_framebuffer_bytes\": " << pixels.size()*sizeof(pixels[0])
                    << ",\n  \"host_draw_buffer_bytes\": " << drawBuffer.size()*sizeof(drawBuffer[0]) << "\n}\n";
                if(!report) throw std::runtime_error("Cannot write host metrics report");
                // Include shift palettes, unknown observations, icon tests, and covered artwork.
                // Blink tests elsewhere require changes to reach pixels; these checks fix time.
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    for(int scenario=0;scenario<6;++scenario) {
                        auto sample=state; Lifecycle sceneLife=life;
                        view.iconTest=scenario==3;
                        if(scenario==1) sample.engineRpm.value=11400;
                        if(scenario==2) sample={};
                        if(scenario==4) { sample.fuelPercent.value=9; sample.left.value=true; }
                        if(scenario==5) { sceneLife=Lifecycle{}; sceneLife.key(true,0); }
                        dashboard.update(sample,view,sceneLife,600); lv_refr_now(nullptr);
                        const auto expected=pixels;
                        flushMetrics={};
                        for(int i=0;i<3;++i) { dashboard.update(sample,view,sceneLife,600); lv_refr_now(nullptr); }
                        if(flushMetrics.calls || pixels!=expected) {
                            std::fprintf(stderr,"Stable scenario: track=%d light=%d scenario=%d flushes=%llu\n",track,light,scenario,static_cast<unsigned long long>(flushMetrics.calls));
                            throw std::runtime_error("Stable layout/theme/warning/lifecycle scenario redrew or changed pixels");
                        }
                    }
                }
                // Parent-owned LVGL objects must disappear when their Dashboard owner dies.
                const auto retainedObjects=countObjects(countObjects,lv_screen_active());
                const auto retainedPixels=pixels;
                for(int cycle=0;cycle<8;++cycle) {
                    {
                        cluster::ui::Dashboard temporary(lv_screen_active());
                        if(countObjects(countObjects,lv_screen_active())!=retainedObjects*2-1)
                            throw std::runtime_error("Dashboard component construction changed object budget");
                        cluster::ui::ViewOptions temporaryView;
                        temporaryView.track=cycle%2; temporaryView.light=(cycle/2)%2;
                        temporary.update(state,temporaryView,life,1400);
                        lv_refr_now(nullptr);
                    }
                    if(countObjects(countObjects,lv_screen_active())!=retainedObjects)
                        throw std::runtime_error("Dashboard destruction leaked LVGL objects");
                    lv_refr_now(nullptr);
                    if(pixels!=retainedPixels)
                        throw std::runtime_error("Dashboard destruction did not restore underlying screen");
                }
                // Hidden telemetry is applied once from the latest snapshot on
                // reveal; an urgent warning must bypass the cover in this update.
                Lifecycle covered; covered.key(true,0);
                view.iconTest=false;
                dashboard.update(state,view,covered,625); lv_refr_now(nullptr);
                auto latest=state; latest.speedKph.value=160; latest.gear.value=6;
                latest.fuelPercent.value=9; latest.currentLapMs.value=91230;
                latest.coolantTemperatureC.value=95;
                flushMetrics={};
                dashboard.update(latest,view,covered,625); lv_refr_now(nullptr);
                if(flushMetrics.calls) throw std::runtime_error("Hidden telemetry caused visible redraw");
                const auto matchesFresh=[&](const ClusterState& sample,const Lifecycle& lifecycle,std::uint64_t now) {
                    dashboard.update(sample,view,lifecycle,now); lv_refr_now(nullptr);
                    const auto revealed=pixels;
                    cluster::ui::Dashboard reference(lv_screen_active());
                    reference.update(sample,view,lifecycle,now); lv_refr_now(nullptr);
                    if(pixels!=revealed) throw std::runtime_error("Deferred or cached presentation differs from latest snapshot");
                };
                latest.engineWarning.value=true;
                matchesFresh(latest,covered,625);
                latest.engineWarning.value=false;
                dashboard.update(latest,view,covered,625); lv_refr_now(nullptr);
                covered.advance(Lifecycle::bootMs);
                matchesFresh(latest,covered,Lifecycle::bootMs);
                // Exercise cache invalidation with units/pages/layouts and every
                // quality, comparing an updated component tree against a fresh one.
                for(auto quality:{Quality::Available,Quality::Unavailable,Quality::Unsupported,Quality::Invalid,Quality::Stale,Quality::Available}) {
                    latest.speedKph.quality=quality; latest.gear.quality=quality;
                    latest.phoneBatteryPercent.quality=quality; latest.averageFuelLPer100Km.quality=quality;
                    latest.realtimeFuelLPer100Km.quality=quality; latest.averageSpeedKph.quality=quality;
                    latest.trip1Km.quality=quality; latest.bestLapMs.quality=quality;
                    latest.currentLapMs.quality=quality; latest.fuelPercent.quality=quality;
                    view.track=!view.track; view.imperial=!view.imperial;
                    view.tripPage=TripPage::Trip1;
                    for(auto page:{InfoPage::RealtimeFuel,InfoPage::AverageFuel,InfoPage::AverageSpeed,InfoPage::Unidentified}) {
                        view.infoPage=page; matchesFresh(latest,life,1400);
                    }
                }
                // Partial dashboard fades must work in both layouts/themes.
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light; view.imperial=false; view.iconTest=false;
                    const auto suffix=std::string(track?"track":"street")+(light?"-day":"-night");
                    Lifecycle reveal; reveal.key(true,0);
                    dashboard.update(state,view,reveal,2775); capture(output/("signature-reveal-"+suffix+".bmp"));
                    Lifecycle ending; ending.key(true,0); ending.advance(Lifecycle::bootMs); ending.key(false,3000);
                    dashboard.update(state,view,ending,3100); capture(output/("signature-fade-"+suffix+".bmp"));
                }
                // Stage boundaries for the R15 light signature with two-second boot and two-second shutdown.
                view={}; view.time="20:55"; view.date="18 Oct 2026";
                const auto renderAnimation=[&](bool bootAnimation,unsigned elapsed,const char* prefix) {
                    Lifecycle animation;
                    animation.key(true,0);
                    std::uint64_t origin=0;
                    if(!bootAnimation) { animation.advance(Lifecycle::bootMs); origin=3500; animation.key(false,origin); }
                    animation.advance(origin+elapsed);
                    dashboard.update(state,view,animation,origin+elapsed);
                    char filename[64]; std::snprintf(filename,sizeof filename,"%s-%04u.bmp",prefix,elapsed);
                    capture(output/filename);
                    if(bootAnimation&&elapsed==1000) {
                        // The wordmark must render, and tapered slashes must not
                        // expand into filled wedges above the intended blades.
                        int white=0;
                        for(int y=235;y<270;++y) for(int x=340;x<460;++x) {
                            const auto pixel=pixels[y*800+x];
                            if((pixel>>11)>24&&((pixel>>5)&63)>48&&(pixel&31)>24) ++white;
                        }
                        if(white<150||pixels[180*800+200]||pixels[180*800+599])
                            throw std::runtime_error("Light-signature wordmark or blade geometry is incorrect");
                    }
                    if(!bootAnimation&&elapsed>=1900) {
                        if(std::any_of(pixels.begin(),pixels.end(),[](std::uint16_t pixel){return pixel!=0;}))
                            throw std::runtime_error("Shutdown final interval is not completely black");
                    }
                };
                for(unsigned elapsed:{0u,100u,250u,349u,350u,500u,700u,850u,1000u,1199u,1200u,1450u,1699u,1700u,1850u,1999u,2000u})
                    renderAnimation(true,elapsed,"boot-stage");
                for(unsigned elapsed:{0u,100u,199u,200u,300u,399u,400u,800u,1000u,1249u,1250u,1500u,1750u,1899u,1900u,1999u,2000u})
                    renderAnimation(false,elapsed,"shutdown-stage");
                if(animationSequence) {
                    for(unsigned elapsed=0;elapsed<=Lifecycle::bootMs;elapsed+=40) renderAnimation(true,elapsed,"boot-motion");
                    for(unsigned elapsed=0;elapsed<=Lifecycle::shutdownMs;elapsed+=40) renderAnimation(false,elapsed,"shutdown-motion");
                }
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view={}; view.track=track; view.light=light; view.time="20:55"; view.date="18 Oct 2026";
                    const std::uint64_t offset=((track?2:0)+(light?1:0))*10000;
                    auto parked=state; parked.speedKph={0,Quality::Available}; parked.gear={0,Quality::Available};
                    const auto suffix=std::string(track?"track":"street")+(light?"-day":"-night");
                    dashboard.update(parked,view,life,offset+6000); lv_refr_now(nullptr);
                    const auto before=pixels;
                    if(!dashboard.touch(view,parked,life,750,40)||!view.settings)
                        throw std::runtime_error("Touch gear failed to open Settings");
                    dashboard.update(parked,view,life,offset+6100);
                    dashboard.update(parked,view,life,offset+6240); capture(output/("settings-enter-"+suffix+".bmp"));
                    dashboard.update(parked,view,life,offset+6380); capture(output/("settings-home-"+suffix+".bmp"));
                    for(unsigned focus=1;focus<4;++focus) {
                        view.settingsFocus=focus; dashboard.update(parked,view,life,offset+6400+focus);
                        capture(output/("settings-focus-"+std::to_string(focus)+"-"+suffix+".bmp"));
                    }
                    view.settingsFocus=0;
                    settingsAction(view,SettingsAction::Select,parked,life);
                    dashboard.update(parked,view,life,offset+6450); capture(output/("appearance-home-"+suffix+".bmp"));
                    if(pixels[207*800+345]!=lv_color_to_u16(lv_color_hex(light?cluster::ui::daylight.background:cluster::ui::midnight.background)))
                        throw std::runtime_error("Live Appearance preview does not follow selected theme");
                    flushMetrics={};
                    for(int idle=0;idle<100;++idle) { dashboard.update(parked,view,life,offset+6450); lv_refr_now(nullptr); }
                    if(flushMetrics.calls) throw std::runtime_error("Settled Appearance redraw without changes");
                    view.appearanceFocus=light?0:1; settingsAction(view,SettingsAction::Select,parked,life);
                    dashboard.update(parked,view,life,offset+6460); capture(output/("appearance-preview-"+suffix+".bmp"));
                    view.appearanceFocus=2; dashboard.update(parked,view,life,offset+6461); capture(output/("appearance-design-"+suffix+".bmp"));
                    view.appearanceFocus=4;
                    if(settingsAction(view,SettingsAction::Select,parked,life)!=SettingsEffect::SavePreferences) throw std::runtime_error("Appearance Apply did not request save");
                    appearanceSaveResult(view,false);
                    dashboard.update(parked,view,life,offset+6462); capture(output/("appearance-save-error-"+suffix+".bmp"));
                    settingsAction(view,SettingsAction::Back,parked,life);
                    if(view.light!=light||view.appearance) throw std::runtime_error("Appearance Cancel did not restore theme");
                    settingsAction(view,SettingsAction::Select,parked,life);
                    view.appearanceFocus=4; appearanceSaveResult(view,true);
                    dashboard.update(parked,view,life,offset+6470); capture(output/("appearance-applied-"+suffix+".bmp"));
                    flushMetrics={};
                    for(int i=0;i<100;++i) { dashboard.update(parked,view,life,offset+6500); lv_refr_now(nullptr); }
                    if(flushMetrics.calls) throw std::runtime_error("Settled settings redraw without changes");
                    auto warning=parked; warning.engineWarning={true,Quality::Available};
                    dashboard.update(warning,view,life,offset+6510); capture(output/("settings-warning-"+suffix+".bmp"));
                    auto unavailable=parked; unavailable.speedKph.quality=Quality::Stale;
                    dashboard.update(unavailable,view,life,offset+6520); capture(output/("settings-unavailable-"+suffix+".bmp"));
                    // Re-enter, return, and compare the exact original dashboard.
                    view.settings=false; dashboard.update(parked,view,life,offset+6600);
                    view.settings=true; dashboard.update(parked,view,life,offset+6700); dashboard.update(parked,view,life,offset+6980);
                    settingsAction(view,SettingsAction::Back,parked,life);
                    dashboard.update(parked,view,life,offset+7000);
                    dashboard.update(parked,view,life,offset+7140); capture(output/("settings-exit-"+suffix+".bmp"));
                    dashboard.update(parked,view,life,offset+7280); lv_refr_now(nullptr);
                    if(before!=pixels) { throw std::runtime_error("Settings exit failed to restore dashboard exactly: "+suffix); }
                }
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view={};view.track=track;view.light=light;view.settings=true;view.phone=true;
                    const auto suffix=std::string(track?"track":"street")+(light?"-day":"-night");
                    const std::uint64_t base=50000+((track?2:0)+(light?1:0))*20000;
                    const char* names[]={"unpaired","saved","connected","forget-confirmation","unavailable","stale","pair-request","pair-approved"};
                    for(unsigned scene=0;scene<8;++scene){
                        auto sample=state;sample.speedKph={0,Quality::Available};sample.gear={0,Quality::Available};
                        auto& phone=sample.phone;phone.quality=Quality::Available;
                        phone.paired=scene>=1&&scene<=5;phone.connected=scene==2||scene==3;
                        std::strcpy(phone.pairingCode.data(),"123456");
                        if(scene>=6){phone.pairingRequested=true;phone.pairingApproved=scene==7;}
                        if(scene==4)phone.quality=Quality::Unavailable;
                        if(scene==5)phone.quality=Quality::Stale;
                        view.phoneForgetConfirmation=scene==3;view.phoneFocus=0;
                        dashboard.update(sample,view,life,base+scene*1000);dashboard.update(sample,view,life,base+scene*1000+280);
                        capture(output/("phone-"+std::string(names[scene])+"-"+suffix+".bmp"));
                        flushMetrics={};for(int idle=0;idle<100;++idle){dashboard.update(sample,view,life,base+scene*1000+300);lv_refr_now(nullptr);}
                        if(flushMetrics.calls)throw std::runtime_error("Settled Phone redraw without changes");
                    }
                    for(unsigned scene=0;scene<8;++scene) {
                        auto sample=state;sample.phone={};auto& phone=sample.phone;
                        phone.bleBench=true;phone.quality=scene==7?Quality::Unavailable:Quality::Available;
                        phone.pairWindowOpen=scene==1;phone.pairingRequested=scene==2||scene==3;
                        phone.pairingApproved=scene==3;if(phone.pairingRequested){std::strcpy(phone.pairingCode.data(),"012345");std::strcpy(phone.displayName.data(),"Samsung S25 Ultra");}
                        phone.paired=scene==4||scene==5;phone.connected=scene==5;phone.commandFailed=scene==6;
                        view.phoneForgetConfirmation=false;view.phoneFocus=0;
                        dashboard.update(sample,view,life,base+11000+scene*1000);
                        capture(output/("ble-phone-"+std::to_string(scene)+"-"+suffix+".bmp"));
                    }
                    auto pairWarning=state;pairWarning.phone={};pairWarning.phone.quality=Quality::Available;
                    pairWarning.phone.bleBench=pairWarning.phone.pairingRequested=true;
                    std::strcpy(pairWarning.phone.pairingCode.data(),"999999");std::snprintf(pairWarning.phone.displayName.data(),pairWarning.phone.displayName.size(),"%s","WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW");pairWarning.engineWarning={true,Quality::Available};
                    view.phoneFocus=1;dashboard.update(pairWarning,view,life,base+19000);
                    capture(output/("ble-phone-modal-warning-"+suffix+".bmp"));
                    auto forgetting=state;forgetting.phone={};forgetting.phone.bleBench=true;
                    forgetting.phone.quality=Quality::Available;forgetting.phone.paired=forgetting.phone.connected=forgetting.phone.forgetting=true;
                    view.phoneForgetConfirmation=false;view.phoneFocus=0;
                    dashboard.update(forgetting,view,life,base+19500);
                    capture(output/("ble-phone-forgetting-"+suffix+".bmp"));
                    auto interruption=state;interruption.speedKph={0,Quality::Available};interruption.engineWarning={true,Quality::Available};
                    reconcileSettings(view,interruption,life);
                    if(!view.settings||!view.phone)throw std::runtime_error("Prototype warning unexpectedly closed Phone");
                    dashboard.update(interruption,view,life,base+9000);capture(output/("phone-warning-"+suffix+".bmp"));
                    view.settings=true;view.phone=true;interruption.engineWarning={false,Quality::Available};interruption.speedKph.quality=Quality::Stale;
                    reconcileSettings(view,interruption,life);dashboard.update(interruption,view,life,base+10000);capture(output/("phone-speed-stale-"+suffix+".bmp"));
                }
                for(bool track:{false,true})for(bool light:{false,true}) {
                    view={};view.track=track;view.light=light;
                    auto sample=state;sample.speedKph.quality=Quality::Unavailable;
                    const auto suffix=std::string(track?"track":"street")+(light?"-day":"-night");
                    const std::uint64_t clockBase=300000+((track?2:0)+(light?1:0))*20000;
                    dashboard.update(sample,view,life,clockBase-1000);dashboard.update(sample,view,life,clockBase-720);
                    const auto palette=light?cluster::ui::daylight:cluster::ui::midnight;
                    for(unsigned status=0;status<3;++status)for(bool twelve:{false,true}) {
                        view.vehicleLink=static_cast<cluster::ui::VehicleLink>(status);
                        view.time=twelve?"12:05":"00:05";view.period=twelve?"AM":"";view.date="29/10/2026";
                        dashboard.update(sample,view,life,clockBase+status*1000);
                        capture(output/("clock-header-"+suffix+"-"+std::to_string(status)+(twelve?"-12":"-24")+".bmp"));
                        const auto expected=lv_color_to_u16(lv_color_hex(status==1?palette.ink:status==2?palette.red:palette.amber));
                        unsigned inkCount=0;for(int y=30;y<62;++y)for(int x=734;x<766;++x)if(pixels[y*800+x]==expected)++inkCount;
                        if(inkCount<10)throw std::runtime_error("Settings gear lost connection-status color");
                    }
                    view.time="12:05";view.period="PM";view.vehicleLink=cluster::ui::VehicleLink::Connected;
                    dashboard.update(sample,view,life,clockBase+3000);capture(output/("clock-header-pm-"+suffix+".bmp"));
                    if(!dashboard.touch(view,sample,life,750,40)||!view.settings)throw std::runtime_error("Unknown speed still blocks Settings");
                    dashboard.update(sample,view,life,clockBase+4000);dashboard.update(sample,view,life,clockBase+4280);
                    if(!dashboard.touch(view,sample,life,600,380)||!view.clock)throw std::runtime_error("Clock tile touch failed");
                    auto clockSample=cluster::presentation::clockText(1793212500,330,view.clockPreferences);
                    view.time=clockSample.time;view.date=clockSample.date;view.period=clockSample.period;
                    dashboard.update(sample,view,life,clockBase+4300);capture(output/("clock-settings-"+suffix+".bmp"));
                    dashboard.touch(view,sample,life,400,208);if(!view.clockPreferences.twelveHour)throw std::runtime_error("12-hour touch choice failed");
                    clockSample=cluster::presentation::clockText(1793212500,330,view.clockPreferences);
                    view.clockSaveFailed=true;dashboard.update(sample,view,life,clockBase+4320);capture(output/("clock-save-error-"+suffix+".bmp"));
                    flushMetrics={};for(int i=0;i<100;++i){dashboard.update(sample,view,life,clockBase+4320);lv_refr_now(nullptr);}
                    if(flushMetrics.calls)throw std::runtime_error("Settled clock page redraw without changes");
                }
                if(warnings) throw std::runtime_error("LVGL render diagnostics reported warnings");
                std::printf("Rendered dashboard, settings and Phone scenarios to %s\n",output.string().c_str());
            } else {
                using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
                using Renderer=std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)>;
                using Texture=std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)>;
                Window window(SDL_CreateWindow("R15 Smart Cluster · LVGL · Simulated",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,480,SDL_WINDOW_ALLOW_HIGHDPI),SDL_DestroyWindow);
                if(!window) throw std::runtime_error(SDL_GetError());
                Renderer renderer(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC),SDL_DestroyRenderer);
                if(!renderer) renderer.reset(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_SOFTWARE));
                if(!renderer) throw std::runtime_error(SDL_GetError());
                SDL_RenderSetLogicalSize(renderer.get(),800,480);
                // Host-only interpolation for Retina enlargement; LVGL still renders 800x480.
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
                Texture texture(SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,800,480),SDL_DestroyTexture);
                if(!texture) throw std::runtime_error(SDL_GetError());
                const auto start=std::chrono::steady_clock::now();
                auto milliseconds=[&]{return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());};
                cluster::host::PhoneBridge phoneBridge(std::getenv("R15_PHONE_BRIDGE_SOCKET"));
                cluster::host::VehicleSerial vehicleSerial(vehiclePort);
                if(vehiclePort) {
                    state=ClusterState{};
                    std::fprintf(stderr,"USB input test: OUT1=left, OUT2=right; gear/high beam configurable.\n");
                }
                if(phoneBridge.enabled()) view.bluetoothDemo=false;
                cluster::host::BluetoothDemo bluetoothDemo;
                cluster::host::WifiDemo wifiDemo;
                const auto dispatchEffect=[&](SettingsEffect effect,std::uint64_t now) {
                    if(effect==SettingsEffect::PhoneApprove||effect==SettingsEffect::PhoneReject||effect==SettingsEffect::PhonePair||effect==SettingsEffect::PhoneDisconnect||effect==SettingsEffect::PhoneForget){
                        phoneBridge.command(effect==SettingsEffect::PhoneApprove?'A':effect==SettingsEffect::PhoneReject?'R':effect==SettingsEffect::PhonePair?'P':effect==SettingsEffect::PhoneDisconnect?'D':'F');
                    } else if(effect==SettingsEffect::SavePreferences||effect==SettingsEffect::SaveClockPreferences) {
                        auto requested=confirmedPreferences;
                        if(effect==SettingsEffect::SaveClockPreferences)requested.clock=view.clockPreferences;else requested.light=view.light;
                        const bool saved=requested==confirmedPreferences||preferenceFile.save(requested);
                        if(saved)confirmedPreferences=requested;
                        if(effect==SettingsEffect::SaveClockPreferences)clockSaveResult(view,saved);else appearanceSaveResult(view,saved);
                    } else if(!phoneBridge.enabled()) {bluetoothDemo.request(effect,state,now);wifiDemo.request(effect,state,view.wifiNetwork,now);}
                };
                life.key(true,0); bool running=true,missing=false; help(); std::uint64_t last=0;
                cluster::presentation::ClockText clockLabels;
                bool repaint=true;
                int bridgeWindowState=-1;bool previousPairOffer=false;
                while(running) {
                    const auto now=milliseconds(); SDL_Event event;
                    while(SDL_PollEvent(&event)) {
                        if(event.type==SDL_RENDER_DEVICE_RESET) {
                            texture.reset(SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,800,480));
                            if(!texture) throw std::runtime_error(SDL_GetError());
                            framebufferDirty=true;
                        }
                        if(event.type==SDL_RENDER_TARGETS_RESET) repaint=true;
                        if(event.type==SDL_WINDOWEVENT &&
                           (event.window.event==SDL_WINDOWEVENT_EXPOSED ||
                            event.window.event==SDL_WINDOWEVENT_SIZE_CHANGED ||
                            event.window.event==SDL_WINDOWEVENT_RESTORED)) repaint=true;
                        if(event.type==SDL_QUIT) running=false;
                        if(event.type==SDL_MOUSEBUTTONUP&&event.button.button==SDL_BUTTON_LEFT) {
                            float x=0,y=0;
                            SDL_RenderWindowToLogical(renderer.get(),event.button.x,event.button.y,&x,&y);
                            if(const auto effect=dashboard.touch(view,missing?ClusterState{}:state,life,int(x),int(y)))
                                dispatchEffect(*effect,now);
                        }
                        if(event.type!=SDL_KEYDOWN) continue;
                        const auto key=event.key.keysym.sym;
                        if(event.key.repeat && key!=SDLK_LEFT&&key!=SDLK_RIGHT&&key!=SDLK_UP&&key!=SDLK_DOWN&&key!=SDLK_LEFTBRACKET&&key!=SDLK_RIGHTBRACKET) continue;
                        if(const auto action=cluster::host::settingsAction(key,view.settings)) {
                            dispatchEffect(settingsAction(view,*action,missing?ClusterState{}:state,life),now);
                            continue;
                        }
                        if(view.settings&&(key==SDLK_RETURN||key==SDLK_t||key==SDLK_TAB)) continue;
                        if(const auto action=cluster::host::dashboardAction(key)) {
                            const auto effect=applyAction(view,*action);
                            // Only the simulator handles this request until a reviewed
                            // command protocol and actual vehicle capability exist.
                            if(effect==InputEffect::RequestTcsToggle&&!missing&&state.tcsEnabled.available())
                                state.tcsEnabled.value=!state.tcsEnabled.value;
                            continue;
                        }
                        switch(key) {
                        case SDLK_ESCAPE: running=false; break;
                        case SDLK_RIGHT: state.speedKph.value=std::min(180,state.speedKph.value+1); break;
                        case SDLK_LEFT: state.speedKph.value=std::max(0,state.speedKph.value-1); break;
                        case SDLK_UP: state.engineRpm.value=std::min(13000,state.engineRpm.value+100); break;
                        case SDLK_DOWN: state.engineRpm.value=std::max(0,state.engineRpm.value-100); break;
                        case SDLK_LEFTBRACKET: state.fuelPercent.value=std::max(0,state.fuelPercent.value-1); break;
                        case SDLK_RIGHTBRACKET: state.fuelPercent.value=std::min(100,state.fuelPercent.value+1); break;
                        case SDLK_p: state.speedKph={0,Quality::Available}; state.gear={0,Quality::Available}; missing=false; break;
                        case SDLK_x: missing=!missing; break;
                        case SDLK_o: life.key(!life.ignition(),now); break;
                        case SDLK_l: state.left.value=!state.left.value; state.right.value=false; break;
                        case SDLK_r: state.right.value=!state.right.value; state.left.value=false; break;
                        case SDLK_z: { const bool on=!(state.left.value&&state.right.value); state.left.value=state.right.value=on; break; }
                        case SDLK_h: state.highBeam.value=!state.highBeam.value; break;
                        case SDLK_a: state.absWarning.value=!state.absWarning.value; break;
                        case SDLK_e: state.engineWarning.value=!state.engineWarning.value; break;
                        case SDLK_c: state.coolantWarning.value=!state.coolantWarning.value; break;
                        case SDLK_SPACE: state=demo(); bluetoothDemo=cluster::host::BluetoothDemo{}; wifiDemo=cluster::host::WifiDemo{}; missing=false; break;
                        case SDLK_F1: help(); break;
                        case SDLK_F5: wifiDemo.failNext(); break;
                        case SDLK_F6: wifiDemo.toggleAvailability(state); break;
                        case SDLK_F3: bluetoothDemo.failNext(); break;
                        case SDLK_F4: bluetoothDemo.toggleAvailability(state); break;
                        case SDLK_F2: view.iconTest=!view.iconTest; break;
                        default: if(key>=SDLK_0&&key<=SDLK_6) state.gear.value=key-SDLK_0; break;
                        }
                    }
                    life.advance(now);
                    state.vva.value=state.engineRpm.value>7500; // Simulator-only display demonstration.
                    const auto wall=std::time(nullptr);
                    std::tm local{};localtime_r(&wall,&local);
                    view.clockSourceOffset=int(local.tm_gmtoff/60);
                    clockLabels=cluster::presentation::clockText(std::uint64_t(wall),view.clockSourceOffset,view.clockPreferences);
                    view.time=clockLabels.time;view.date=clockLabels.date;view.period=clockLabels.period;
                    lv_tick_inc(static_cast<std::uint32_t>(now-last)); last=now;
                    if(phoneBridge.enabled()) {
                        const bool connected=phoneBridge.update(now);
                        state.phone=phoneBridge.phone();
                        if(state.phone.available()&&state.phone.pairingRequested&&!previousPairOffer&&state.speedKph.available()&&state.speedKph.value==0&&canOpenSettings(state,life)){
                            view.settings=true;view.phone=true;view.settingsFocus=1;view.phoneFocus=0;
                        }
                        previousPairOffer=state.phone.available()&&state.phone.pairingRequested;
                        state.phoneConnected={connected,phoneBridge.fresh()?Quality::Available:Quality::Unavailable};
                        state.phoneBatteryPercent.quality=Quality::Unavailable;
                        state.bluetooth.quality=Quality::Unavailable;
                        state.wifi.quality=Quality::Unavailable;
                        const int bridgePhase=phoneBridge.fresh()?int(connected):-2;
                        if(bridgeWindowState!=bridgePhase) {
                        bridgeWindowState=bridgePhase;
                        SDL_SetWindowTitle(window.get(),!phoneBridge.fresh()?"R15 Preview - Phone bridge unavailable - SIMULATED VEHICLE":connected ? "R15 Preview - Wi-Fi phone connected - SIMULATED VEHICLE" : "R15 Preview - Wi-Fi phone disconnected - SIMULATED VEHICLE");
                        }
                    } else {bluetoothDemo.update(state,now);wifiDemo.update(state,now);}
                    if(vehiclePort) {
                        // Replace demo mutations before presentation. Only received discrete inputs are presented.
                        state=ClusterState{};
                        state=vehicleSerial.updateState(now);
                        const auto clicks=vehicleSerial.takeClicks();
                        for(unsigned i=0;i<clicks.trip;++i)applyAction(view,DashboardAction::NextTrip);
                        for(unsigned i=0;i<clicks.info;++i)applyAction(view,DashboardAction::NextInfo);
                        missing=false;view.iconTest=false;view.bluetoothDemo=false;view.simulated=false;
                        view.vehicleLink=vehicleSerial.fresh(now)?cluster::ui::VehicleLink::Connected:vehicleSerial.received()?cluster::ui::VehicleLink::Stale:cluster::ui::VehicleLink::Waiting;
                        SDL_SetWindowTitle(window.get(),"R15 Preview - USB discrete input test");
                    }
                    auto observed=missing?ClusterState{}:state;
                    if(!valid(observed)) throw std::runtime_error("Simulator produced invalid normalized state");
                    dispatchEffect(reconcileSettings(view,observed,life),now);
                    observed=missing?ClusterState{}:state;
                    dashboard.update(observed,view,life,now); lv_timer_handler();
                    presentFramebuffer(renderer.get(),texture.get(),repaint);
                    std::this_thread::sleep_for(std::chrono::milliseconds(8));
                }
            }
        }
        lv_display_delete(display); lv_deinit(); return 0;
    } catch(const std::exception& e) { std::fprintf(stderr,"Preview failed: %s\n",e.what()); return 1; }
}
