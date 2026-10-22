#include "embedded/ui/Dashboard.h"
#include "firmware/display/main/BenchSweep.h"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
uint32_t ticks=0;
unsigned warnings=0;
struct Surface {
    std::vector<uint16_t> first=std::vector<uint16_t>(800*480);
    std::vector<uint16_t> second=std::vector<uint16_t>(800*480);
    std::vector<uint16_t> presented=std::vector<uint16_t>(800*480);
    lv_display_t* display=nullptr;
    std::unique_ptr<cluster::ui::Dashboard> dashboard;
    uint64_t dirtyPixels=0;
    unsigned frames=0;
    explicit Surface(lv_display_render_mode_t mode) {
        display=lv_display_create(800,480);
        lv_display_set_default(display);
        lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
        lv_display_set_user_data(display,this);
        lv_display_set_buffers(display,first.data(),second.data(),first.size()*2,mode);
        lv_display_set_flush_cb(display,[](lv_display_t* display,const lv_area_t* area,uint8_t* pixels) {
            auto& self=*static_cast<Surface*>(lv_display_get_user_data(display));
            self.dirtyPixels+=lv_area_get_size(area);
            if(lv_display_flush_is_last(display)) {
                auto* data=reinterpret_cast<uint16_t*>(pixels);
                if(data!=self.first.data()&&data!=self.second.data()) throw std::runtime_error("Not a full framebuffer base");
                std::copy(data,data+self.presented.size(),self.presented.begin());
                ++self.frames;
            }
            lv_display_flush_ready(display);
        });
        dashboard=std::make_unique<cluster::ui::Dashboard>(lv_screen_active());
    }
    ~Surface() { dashboard.reset(); lv_display_delete(display); }
    void update(const cluster::ClusterState& state,const cluster::ui::ViewOptions& view,const cluster::Lifecycle& life) {
        lv_display_set_default(display);
        dashboard->update(state,view,life,ticks);
        lv_refr_now(display);
    }
};
}
int main() {
    lv_init();
    lv_tick_set_cb([](){ return ticks; });
    lv_log_register_print_cb([](lv_log_level_t,const char* message){ ++warnings; std::fprintf(stderr,"%s",message); });
    Surface full(LV_DISPLAY_RENDER_MODE_FULL), direct(LV_DISPLAY_RENDER_MODE_DIRECT);
    cluster::Lifecycle life; life.key(true,0); life.advance(3000);
    cluster::ui::ViewOptions view;
    auto state=cluster::board::benchSweep(6000);
    const auto compare=[&] {
        full.update(state,view,life); direct.update(state,view,life);
        if(full.presented!=direct.presented) throw std::runtime_error("DIRECT differs from FULL pixels");
    };
    ticks=3000; compare();
    full.dirtyPixels=direct.dirtyPixels=0;
    for(unsigned step=0;step<30;++step) {
        ticks+=40; state.engineRpm.value=6000+step*10; state.speedKph.value=60+step;
        compare();
    }
    if(direct.dirtyPixels>=full.dirtyPixels) throw std::runtime_error("Dirty rendering did not reduce redraw area");
    std::printf("RPM/speed updates: FULL pixels=%llu DIRECT pixels=%llu (host, not panel FPS)\n",
        static_cast<unsigned long long>(full.dirtyPixels),static_cast<unsigned long long>(direct.dirtyPixels));
    for(unsigned scenario=0;scenario<64;++scenario) {
        ticks+=100;
        state=cluster::board::benchSweep(scenario*400);
        view.light=(scenario%2)!=0; view.track=(scenario%4)>=2;
        view.iconTest=(scenario%7)==0;
        if(scenario%9==0) state={};
        compare();
    }
    // Lifecycle covers, urgent interruption and black endpoints must also match.
    view={}; state={}; life={}; ticks+=100; life.key(true,ticks);
    const auto start=ticks;
    for(unsigned offset:{0u,525u,1275u,1800u,2550u,3000u}) {
        ticks=start+offset; life.advance(ticks); compare();
    }
    life.key(false,ticks); const auto shutdown=ticks;
    for(unsigned offset:{0u,200u,400u,1250u,1900u,2000u}) {
        ticks=shutdown+offset; life.advance(ticks); compare();
    }
    state.engineWarning={true,cluster::Quality::Available}; ticks+=20; compare();
    if(warnings) throw std::runtime_error("LVGL diagnostics during DIRECT comparison");
}
