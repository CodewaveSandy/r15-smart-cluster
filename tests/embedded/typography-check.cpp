#include "embedded/ui/Typography.h"
#include "embedded/ui/Icons.h"
#include "embedded/ui/Geometry.h"
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace cluster::ui;
    lv_init();
    int failed=0;
    const auto fits=[&](const lv_font_t* font,const char* value,int width) {
        lv_point_t bounds{}; lv_text_get_size(&bounds,value,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
        if(bounds.x>width) { std::fprintf(stderr,"Text exceeds %d px: %s (%d px)\n",width,value,int(bounds.x)); ++failed; }
    };
    fits(fonts::value,"RPM × 1,000",geometry::TachometerLayout::caption.width);
    fits(fonts::value,"30000 rpm",geometry::TachometerLayout::value.width);
    fits(fonts::heading,"STREET",geometry::HeaderLayout::mode.width);
    fits(fonts::label,"BATTERY VOLTAGE",geometry::RideInfoLayout::infoTitle.width);
    fits(fonts::heading,"1000000000.0",geometry::RideInfoLayout::tripValue.width);
    fits(fonts::heading,"OFF",geometry::RideInfoLayout::infoValue.width);
    fits(fonts::clock,"23:59",geometry::HeaderLayout::clock.width);
    fits(fonts::speed,"180",geometry::ReadoutLayout::streetSpeed.width);
    fits(fonts::track,"99:59.99",geometry::ReadoutLayout::lap.width);
    fits(fonts::heading,"99:59.99",geometry::RideInfoLayout::tripValue.width);
    fits(fonts::track,"—:—.—",geometry::ReadoutLayout::lap.width);
    fits(fonts::secondary,"400",geometry::ReadoutLayout::trackSpeed.width);
    for(const auto* gear:{"N","1","2","3","4","5","6","—"}) fits(fonts::gear,gear,geometry::HeaderLayout::gear.width);
    if(fonts::gear->line_height>60) { std::fprintf(stderr,"Gear exceeds header height\n"); ++failed; }
    fits(fonts::caption,"CLOCK UNAVAILABLE",geometry::HeaderLayout::date.width);
    fits(fonts::label,"DATA UNAVAILABLE",geometry::HeaderLayout::subtitle.width);
    for(const auto* font:{fonts::caption,fonts::label,fonts::value,fonts::unit,fonts::heading}) {
        for(unsigned code:{176u,183u,215u,8212u}) {
            lv_font_glyph_dsc_t glyph{};
            if(!lv_font_get_glyph_dsc(font,&glyph,code,0)) { std::fprintf(stderr,"Missing text glyph U+%X\n",code); ++failed; }
        }
    }
    for(const auto& icon:iconDefinitions) {
        lv_font_glyph_dsc_t glyph{};
        if(!lv_font_get_glyph_dsc(iconFont(icon.kind),&glyph,icon.codepoint,0)||glyph.box_w==0||glyph.box_h==0||glyph.box_w>32||glyph.box_h>32) {
            std::fprintf(stderr,"Missing or out-of-bounds icon U+%X\n",unsigned(icon.codepoint)); ++failed;
        }
    }
    lv_deinit(); return failed?1:0;
}
