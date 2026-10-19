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
    fits(fonts::rpm,"30,000",geometry::TachometerLayout::value.width);
    fits(fonts::value,"RPM",geometry::TachometerLayout::unit.width);
    fits(fonts::heading,"STREET",geometry::HeaderLayout::mode.width);
    fits(fonts::label,"BATTERY VOLTAGE",geometry::RideInfoLayout::infoTitle.width);
    fits(fonts::heading,"1000000000.0",geometry::RideInfoLayout::tripValue.width);
    fits(fonts::heading,"OFF",geometry::RideInfoLayout::infoValue.width);
    fits(fonts::clock,"23:59",geometry::HeaderLayout::clock.width);
    fits(fonts::speed,"400",geometry::ReadoutLayout::streetSpeed.width);
    fits(fonts::unit,"km/h",geometry::ReadoutLayout::trackUnit.width);
    fits(fonts::unit,"mph",geometry::ReadoutLayout::trackUnit.width);
    fits(fonts::track,"99:59.99",geometry::ReadoutLayout::lap.width);
    fits(fonts::heading,"99:59.99",geometry::RideInfoLayout::tripValue.width);
    fits(fonts::track,"—:—.—",geometry::ReadoutLayout::lap.width);
    fits(fonts::secondary,"400",geometry::ReadoutLayout::trackSpeed.width);
    for(const auto* gear:{"N","1","2","3","4","5","6","—"}) fits(fonts::gear,gear,geometry::HeaderLayout::gear.width);
    if(fonts::gear->line_height>60) { std::fprintf(stderr,"Gear exceeds header height\n"); ++failed; }
    fits(fonts::caption,"CLOCK UNAVAILABLE",geometry::HeaderLayout::date.width);
    fits(fonts::label,"DATA UNAVAILABLE",geometry::HeaderLayout::subtitle.width);
    fits(fonts::label,"REAL-TIME FUEL",geometry::RideInfoLayout::infoTitle.width);
    fits(fonts::heading,"1000.0 L/100km",geometry::RideInfoLayout::infoValue.width);
    fits(fonts::heading,"400.0 km/h",geometry::RideInfoLayout::infoValue.width);
    fits(fonts::label,"RESERVE",geometry::FuelLayout::title.width);
    for(const auto* font:{fonts::caption,fonts::label,fonts::value,fonts::unit,fonts::heading}) {
        for(unsigned code:{176u,183u,215u,8212u}) {
            lv_font_glyph_dsc_t glyph{};
            if(!lv_font_get_glyph_dsc(font,&glyph,code,0)) { std::fprintf(stderr,"Missing text glyph U+%X\n",code); ++failed; }
        }
    }
    const auto requiresGlyphs=[&](const lv_font_t* font,const char* ascii) {
        for(const unsigned char* c=reinterpret_cast<const unsigned char*>(ascii);*c;++c) {
            lv_font_glyph_dsc_t glyph{};
            if(!lv_font_get_glyph_dsc(font,&glyph,*c,0)) { std::fprintf(stderr,"Missing required instrument glyph %c\n",*c); ++failed; }
        }
        lv_font_glyph_dsc_t dash{};
        if(!lv_font_get_glyph_dsc(font,&dash,8212,0)) { std::fprintf(stderr,"Missing instrument unavailable dash\n"); ++failed; }
    };
    requiresGlyphs(fonts::rpm,"0123456789,");
    requiresGlyphs(fonts::gear,"123456N");
    requiresGlyphs(fonts::speed,"0123456789");
    requiresGlyphs(fonts::track,"0123456789:.");
    requiresGlyphs(fonts::clock,"0123456789:");
    requiresGlyphs(fonts::secondary,"0123456789");
    if(lv_font_default()!=fonts::label) { std::fprintf(stderr,"Default font does not reuse bundled Inter\n"); ++failed; }
    for(const auto& icon:iconDefinitions) {
        lv_font_glyph_dsc_t glyph{};
        if(!lv_font_get_glyph_dsc(iconFont(icon.kind),&glyph,icon.codepoint,0)||glyph.box_w==0||glyph.box_h==0||glyph.box_w>32||glyph.box_h>32) {
            std::fprintf(stderr,"Missing or out-of-bounds icon U+%X\n",unsigned(icon.codepoint)); ++failed;
        }
    }
    lv_deinit(); return failed?1:0;
}
