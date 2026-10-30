#include "embedded/ui/Typography.h"
#include "embedded/ui/ReadoutText.h"
#include "embedded/ui/Icons.h"
#include "embedded/ui/Geometry.h"
#include "embedded/ui/TouchNavigation.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace cluster::ui;
    {
        using namespace cluster;
        Lifecycle life; life.key(true,0); life.advance(2000);
        ClusterState state; DisplayOptions view;
        settingsTouch(view,state,life,750,40,true); assert(view.settings);
        settingsTouch(view,state,life,750,40,true); assert(!view.settings);
        state.speedKph={0,Quality::Available};
        settingsTouch(view,state,life,750,40,true); assert(view.settings);
        settingsTouch(view,state,life,100,240,false); assert(!view.appearance);
        settingsTouch(view,state,life,100,240,true); assert(view.appearance);
        settingsTouch(view,state,life,100,310,true); assert(view.light);
        settingsTouch(view,state,life,500,440,true); assert(!view.appearance&&!view.light);
        settingsTouch(view,state,life,750,155,true); assert(!view.settings);
        state.speedKph={20,Quality::Available};
        settingsTouch(view,state,life,750,40,true); assert(view.settings);
        settingsTouch(view,state,life,750,40,true); assert(!view.settings);
        state.speedKph={0,Quality::Available}; state.engineWarning={true,Quality::Available};
        settingsTouch(view,state,life,750,40,true); assert(view.settings);
        assert(!settingsTouch(view,state,life,800,40,true));
        view.phone=true;state.phone.quality=Quality::Available;
        state.phone.paired=state.phone.connected=true;
        const auto row=geometry::PhoneLayout::row(1);
        const int rowY=geometry::SettingsLayout::bounds.y+row.y+10;
        settingsTouch(view,state,life,row.x+10,rowY,true);assert(view.phoneForgetConfirmation);
        auto confirmed=settingsTouch(view,state,life,row.x+10,rowY,true);
        assert(confirmed&&*confirmed==SettingsEffect::PhoneForget&&!view.phoneForgetConfirmation);
        state.phone.forgetting=true;
        for(int repeat=0;repeat<5;++repeat) {
            auto repeated=settingsTouch(view,state,life,row.x+10,rowY,true);
            assert(repeated&&*repeated==SettingsEffect::None&&!view.phoneForgetConfirmation);
        }
        state.phone.forgetting=false;state.phone.paired=state.phone.connected=false;
        reconcileSettings(view,state,life);assert(phoneMenu(state.phone,false).items[0]==PhoneItem::Pair);

    }
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
    fits(fonts::heading,"WWWWWWWWWWWWWWWWWWWWWWWW",650);
    fits(fonts::value,"Cancel pairing / connection",650);
    fits(fonts::label,"Connected - last request failed",650);
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
    fits(fonts::value,"WWWWWWWWWWWWWWWWWWWWWWWW",520);
    fits(fonts::label,"Credentials pending - Enter simulates connection",650);
    fits(fonts::heading,"Forget connected network?",650);
    fits(fonts::value,"Protected",160);
    {
        auto* display=lv_display_create(800,480);
        ReadoutText storage;
        auto* label=lv_label_create(lv_screen_active());
        lv_label_set_long_mode(label,LV_LABEL_LONG_CLIP);
        storage.update(label,"0");
        const auto* address=lv_label_get_text(label);
        // Alternate lengths, UTF-8 unavailable text, and the full buffer capacity.
        const char values[][32]={"13,000","0","—","99:59.99","1234567890123456789012345678901"};
        for(int iteration=0;iteration<1000;++iteration) {
            const auto& value=values[iteration%5];
            storage.update(label,value);
            storage.update(label,value);
            if(lv_label_get_text(label)!=address || std::strcmp(lv_label_get_text(label),value)) {
                std::fprintf(stderr,"Readout storage moved or text was corrupted\n"); ++failed; break;
            }
        }
        lv_obj_delete(label); // Storage outlives the LVGL label.
        lv_display_delete(display);
    }
    lv_deinit(); return failed?1:0;
}
