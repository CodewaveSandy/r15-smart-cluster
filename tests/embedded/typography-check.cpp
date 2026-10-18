#include "embedded/ui/Typography.h"
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
    fits(fonts::value,"RPM × 1,000",160);
    fits(fonts::value,"30000 rpm",162);
    fits(fonts::heading,"STREET",160);
    fits(fonts::label,"BATTERY VOLTAGE",260);
    fits(fonts::heading,"1000000000.0",170);
    fits(fonts::heading,"OFF",265);
    fits(fonts::clock,"23:59",122);
    fits(fonts::speed,"180",375);
    fits(fonts::gear,"N",65);
    fits(fonts::caption,"CLOCK UNAVAILABLE",142);
    fits(fonts::label,"DATA UNAVAILABLE",200);
    for(const auto* font:{fonts::caption,fonts::label,fonts::value,fonts::unit,fonts::heading}) {
        for(unsigned code:{176u,183u,215u,8212u}) {
            lv_font_glyph_dsc_t glyph{};
            if(!lv_font_get_glyph_dsc(font,&glyph,code,0)) { std::fprintf(stderr,"Missing text glyph U+%X\n",code); ++failed; }
        }
    }
    lv_deinit(); return failed?1:0;
}
