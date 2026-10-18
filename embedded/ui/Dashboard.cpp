#include "Dashboard.h"
#include "InstrumentPresentation.h"
#include "Typography.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace cluster::ui {
namespace {
lv_color_t color(std::uint32_t c) { return lv_color_hex(c); }
lv_obj_t* box(lv_obj_t* parent,int x,int y,int w,int h) {
    auto* o=lv_obj_create(parent); lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE); return o;
}
lv_obj_t* label(lv_obj_t* parent,int x,int y,int w,const char* text,const lv_font_t* font=fonts::label) {
    auto* o=lv_label_create(parent); lv_obj_set_pos(o,x,y); lv_obj_set_width(o,w);
    lv_obj_set_style_text_font(o,font,0); lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP);
    lv_label_set_text(o,text); return o;
}
void text(lv_obj_t* o,const char* s) { if(std::strcmp(lv_label_get_text(o),s)) lv_label_set_text(o,s); }
void paint(lv_obj_t* o,std::uint32_t c) { lv_obj_set_style_bg_color(o,color(c),0); lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0); }
void ink(lv_obj_t* o,std::uint32_t c) { lv_obj_set_style_text_color(o,color(c),0); }
void show(lv_obj_t* o,bool yes) { if(yes) lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN); }
void alignRight(lv_obj_t* o) { lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_RIGHT,0); }
// Compact static geometry; line points are owned by each icon and never allocated on updates.
struct IconPath { lv_point_precise_t points[18]; std::uint32_t count; };
const IconPath arrow{{{3,16},{14,5},{14,11},{29,11},{29,21},{14,21},{14,27},{3,16}},8};
const IconPath rightArrow{{{29,16},{18,5},{18,11},{3,11},{3,21},{18,21},{18,27},{29,16}},8};
const IconPath engine{{{3,13},{8,13},{8,9},{14,9},{14,6},{21,6},{21,9},{25,9},{25,13},{29,13},{29,24},{24,24},{21,27},{9,27},{6,23},{3,23},{3,13}},17};
const IconPath beam{{{3,7},{10,7},{14,6},{22,6},{28,11},{28,21},{22,26},{14,26},{14,6}},9};
const IconPath temp{{{15,4},{15,21},{19,24},{15,27},{11,24},{15,21}},6};
const IconPath hazard{{{16,3},{30,28},{2,28},{16,3},{16,10},{16,20}},6};
const IconPath phone{{{9,2},{23,2},{23,30},{9,30},{9,2},{13,5},{19,5}},7};
const IconPath pump{{{5,28},{5,3},{19,3},{19,28},{3,28},{21,28},{19,15},{23,15},{23,25},{27,27},{29,25},{29,10},{25,6}},13};
lv_obj_t* icon(lv_obj_t* parent,int x,int y,const IconPath& path) {
    auto* o=lv_line_create(parent); lv_obj_set_pos(o,x,y); lv_obj_set_size(o,32,32);
    lv_line_set_points(o,path.points,path.count); lv_obj_set_style_line_width(o,2,0);
    lv_obj_set_style_line_rounded(o,true,0); return o;
}
void drawFuelSegment(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    lv_area_t a; lv_obj_get_coords(obj,&a);
    lv_draw_triangle_dsc_t d; lv_draw_triangle_dsc_init(&d);
    d.bg_color=lv_obj_get_style_bg_color(obj,0); d.bg_opa=255;
    d.p[0]={a.x1+Metrics::fuelSkew,a.y1}; d.p[1]={a.x2,a.y1}; d.p[2]={a.x2-Metrics::fuelSkew,a.y2};
    lv_draw_triangle(lv_event_get_layer(event),&d);
    d.p[1]=d.p[2]; d.p[2]={a.x1,a.y2};
    lv_draw_triangle(lv_event_get_layer(event),&d);
}
void lamp(lv_obj_t* o,const Signal<bool>& signal,bool phase,std::uint32_t active,const Palette& p) {
    const bool known=signal.available(), lit=known && signal.value && phase;
    const bool unsupported=signal.quality==Quality::Unsupported;
    const auto c=known ? (lit?active:p.muted):unsupported?p.muted:p.amber;
    lv_obj_set_style_line_color(o,color(c),0); ink(o,c);
    lv_obj_set_style_opa(o,known?(lit?255:31):unsupported?31:150,0);
}
}
Dashboard::Dashboard(lv_obj_t* parent) {
    root_=box(parent,0,0,800,480);
    header_=box(root_,0,0,800,85);
    gear_=label(header_,28,26,65,"—",fonts::gear);
    mode_=label(header_,120,26,160,"STREET",fonts::heading);
    subtitle_=label(header_,121,56,200,"ROAD / EVERYDAY");
    phone_=icon(header_,456,31,phone);
    phoneBattery_=label(header_,508,36,100,"PHONE  —");
    qs_=label(header_,288,36,48,"QS"); vva_=label(header_,344,36,60,"VVA");
    clock_=label(header_,650,22,122,"—:—",fonts::clock); alignRight(clock_);
    date_=label(header_,630,57,142,"CLOCK UNAVAILABLE",fonts::caption); alignRight(date_);
    rules_[0]=box(root_,28,85,744,1); rules_[1]=box(root_,96,22,1,48);
    lamps_[0]=icon(root_,32,98,arrow);
    lamps_[1]=label(root_,104,101,40,"N",fonts::heading);
    lamps_[2]=icon(root_,171,98,beam);
    lamps_[3]=label(root_,240,108,45,"TCS");
    lamps_[4]=label(root_,311,108,45,"ABS");
    lamps_[5]=icon(root_,384,98,engine);
    lamps_[6]=icon(root_,455,98,temp);
    lamps_[7]=label(root_,526,108,45,"MSG");
    lamps_[8]=label(root_,597,108,45,"CALL");
    lamps_[9]=icon(root_,668,98,hazard);
    lamps_[10]=icon(root_,736,98,rightArrow);
    for(int i=0;i<14;++i) { char b[8]; std::snprintf(b,sizeof b,"%d",i); ticks_[i]=label(root_,28+i*744/13,144,30,b,fonts::caption); }
    rpmBed_=box(root_,28,164,744,44);
    rpmClip_=box(rpmBed_,0,0,1,44);
    rpmRamp_=box(rpmClip_,0,0,744,44);
    rampGradient_.dir=LV_GRAD_DIR_HOR; rampGradient_.stops_count=6;
    const std::uint32_t colors[]={0x4cdb88,0x4cdb88,0xf5d45b,0xf5d45b,0xf06464,0xf06464};
    const int stops[]={0,3500,4500,7500,8500,13000};
    for(int i=0;i<6;++i) { rampGradient_.stops[i].color=color(colors[i]); rampGradient_.stops[i].opa=255; rampGradient_.stops[i].frac=stops[i]*255/13000; }
    paint(rpmRamp_,colors[0]); lv_obj_set_style_bg_grad(rpmRamp_,&rampGradient_,0);
    label(root_,28,220,160,"RPM × 1,000",fonts::value);
    rpmValue_=label(root_,610,220,162,"— rpm",fonts::value); alignRight(rpmValue_);
    speed_=label(root_,Metrics::speedX,240,Metrics::speedWidth,"—",fonts::speed);
    lv_obj_set_style_text_align(speed_,LV_TEXT_ALIGN_CENTER,0);
    unit_=label(root_,Metrics::speedX,338,Metrics::speedWidth,"km/h",fonts::unit);
    lv_obj_set_style_text_align(unit_,LV_TEXT_ALIGN_CENTER,0);
    lap_=label(root_,560,269,212,"—:—.—",fonts::gear);
    lapHint_=label(root_,560,247,212,"LAP / UNAVAILABLE");
    lowerRule_=box(root_,28,376,744,1);
    tripTitle_=label(root_,28,386,220,"ODO");
    tripValue_=label(root_,28,400,170,"—",fonts::heading);
    tripUnit_=label(root_,203,411,55,"km");
    infoTitle_=label(root_,267,386,260,"COOLANT");
    infoValue_=label(root_,267,402,265,"— V / —°C",fonts::heading);
    fuelDivider_=box(root_,537,387,1,40);
    fuelTitle_=label(root_,583,386,110,"F U E L");
    fuelValue_=label(root_,690,379,82,"—",fonts::heading); alignRight(fuelValue_);
    pump_=icon(root_,550,401,pump);
    for(int i=0;i<Metrics::fuelSegmentCount;++i) {
        const int x=Metrics::fuelX+i*Metrics::fuelWidth/Metrics::fuelSegmentCount;
        const int nextX=Metrics::fuelX+(i+1)*Metrics::fuelWidth/Metrics::fuelSegmentCount;
        fuel_[i]=box(root_,x,Metrics::fuelY(false),nextX-x,12); lv_obj_add_event_cb(fuel_[i],drawFuelSegment,LV_EVENT_DRAW_MAIN,nullptr); }
    overlay_=box(root_,0,0,800,480);
    animationTitle_=label(overlay_,100,160,600,"R15",fonts::gear);
    lv_obj_set_style_text_align(animationTitle_,LV_TEXT_ALIGN_CENTER,0);
    animationSubtitle_=label(overlay_,100,230,600,"SMART CLUSTER");
    lv_obj_set_style_text_align(animationSubtitle_,LV_TEXT_ALIGN_CENTER,0);
    animationBed_=box(overlay_,240,285,320,3);
    animationFill_=box(animationBed_,0,0,1,3);
    shiftGradient_.dir=LV_GRAD_DIR_VER; shiftGradient_.stops_count=3;
    const std::uint32_t shiftColors[]={0x492731,0xb34c48,0x64303b};
    for(int i=0;i<3;++i) { shiftGradient_.stops[i].color=color(shiftColors[i]); shiftGradient_.stops[i].opa=255; shiftGradient_.stops[i].frac=i*127; }
}
Dashboard::~Dashboard() { lv_obj_delete(root_); }
void Dashboard::update(const ClusterState& s,const ViewOptions& v,const Lifecycle& life,std::uint64_t now) {
    const bool shift=shiftLit(s.engineRpm,now);
    auto p=v.light?daylight:midnight;
    if(shift) { p.ink=midnight.ink; p.muted=0xf0d5d0; p.panel=0x352730; p.accent=midnight.accent; }
    paint(root_,shift?0x492731:p.background);
    lv_obj_set_style_bg_grad(root_,shift?&shiftGradient_:nullptr,0);
    lv_obj_set_style_text_color(root_,color(p.ink),0);
    for(auto* r:rules_) paint(r,p.line);
    for(auto* t:ticks_) ink(t,p.muted);
    for(auto* t:{subtitle_,date_,tripUnit_,infoTitle_}) ink(t,p.muted);
    ink(tripTitle_,p.accent); ink(fuelTitle_,p.muted); ink(unit_,p.muted);
    // Presentation-only lamp test; observations remain unchanged.
    const auto showLamp=[&](lv_obj_t* obj,const Signal<bool>& signal,bool phase,std::uint32_t active,const Palette& palette) {
        lamp(obj,v.iconTest?Signal<bool>{true,Quality::Available}:signal,v.iconTest||phase,active,palette);
    };
    char b[96];
    if(s.gear.available()) { if(s.gear.value==0) std::snprintf(b,sizeof b,"N"); else std::snprintf(b,sizeof b,"%d",s.gear.value); } else std::snprintf(b,sizeof b,"—");
    text(gear_,b); ink(gear_,s.gear.available()&&s.gear.value==0?p.accent:p.ink);
    text(mode_,v.track?"TRACK":"STREET");
    const bool unavailable=!s.speedKph.available()||!s.engineRpm.available()||!s.gear.available();
    text(subtitle_,v.iconTest?"ICON TEST":unavailable?"DATA UNAVAILABLE":v.track?"LAP / PERFORMANCE":"ROAD / EVERYDAY");
    ink(subtitle_,unavailable?p.amber:p.muted);
    text(clock_,v.time); text(date_,v.date);
    show(qs_,v.iconTest||(s.quickShifter.available()&&s.quickShifter.value)); ink(qs_,p.accent);
    show(vva_,v.iconTest||(s.vva.available()&&s.vva.value)); ink(vva_,p.amber);
    showLamp(phone_,s.phoneConnected,true,p.accent,p);
    if(s.phoneConnected.available()&&s.phoneConnected.value&&s.phoneBatteryPercent.available()) std::snprintf(b,sizeof b,"PHONE %d%%",s.phoneBatteryPercent.value);
    else std::snprintf(b,sizeof b,"PHONE —"); text(phoneBattery_,b); ink(phoneBattery_,p.muted);
    const bool turn=(now/Metrics::turnPeriod)%2==0;
    showLamp(lamps_[0],s.left,turn,p.accent,p); showLamp(lamps_[10],s.right,turn,p.accent,p);
    showLamp(lamps_[1],Signal<bool>{s.gear.value==0,s.gear.quality},true,p.accent,p);
    showLamp(lamps_[2],s.highBeam,true,p.beam,p); showLamp(lamps_[4],s.absWarning,true,p.amber,p);
    showLamp(lamps_[5],s.engineWarning,true,p.amber,p); showLamp(lamps_[6],s.coolantWarning,true,p.red,p);
    for(int i:{3,7,8}) showLamp(lamps_[i],Signal<bool>{false,Quality::Unsupported},true,p.amber,p);
    showLamp(lamps_[9],Signal<bool>{s.left.value&&s.right.value,s.left.available()&&s.right.available()?Quality::Available:Quality::Unavailable},turn,p.red,p);
    const int h=v.track?52:44;
    lv_obj_set_height(rpmBed_,h); paint(rpmBed_,p.panel); lv_obj_set_height(rpmClip_,h); lv_obj_set_height(rpmRamp_,h);
    const int fill=s.engineRpm.available()?std::clamp(s.engineRpm.value,0,13000)*744/13000:0;
    show(rpmClip_,fill>0); lv_obj_set_width(rpmClip_,std::max(1,fill));
    if(s.engineRpm.available()) std::snprintf(b,sizeof b,"%d rpm",s.engineRpm.value); else std::snprintf(b,sizeof b,"— rpm"); text(rpmValue_,b);
    if(s.speedKph.available()) std::snprintf(b,sizeof b,"%d",int(std::lround(s.speedKph.value*(v.imperial?0.621371:1)))); else std::snprintf(b,sizeof b,"—"); text(speed_,b);
    lv_obj_set_style_text_font(speed_,v.track?fonts::track:fonts::speed,0);
    lv_obj_set_pos(speed_,Metrics::speedX,240);
    lv_obj_set_pos(unit_,Metrics::speedX,v.track?305:338); text(unit_,v.imperial?"mph":"km/h");
    show(lap_,v.track); show(lapHint_,v.track); ink(lapHint_,p.accent);
    lv_obj_set_y(lowerRule_,v.track?336:376); paint(lowerRule_,p.line);
    const int titleY=v.track?379:399;
    const int valueY=titleY+22;
    lv_obj_set_y(tripTitle_,titleY); lv_obj_set_y(tripValue_,valueY);
    text(tripTitle_,tripTitle(v.tripPage));
    const auto distance=tripReading(s,v.tripPage);
    if(distance.available()) std::snprintf(b,sizeof b,"%.1f",distance.value*(v.imperial?0.621371:1));
    else std::snprintf(b,sizeof b,"—");
    text(tripValue_,b); text(tripUnit_,v.imperial?"mi":"km"); lv_obj_set_y(tripUnit_,valueY+10);
    lv_obj_set_y(infoTitle_,titleY); lv_obj_set_y(infoValue_,valueY);
    switch(v.infoPage) {
    case InfoPage::Coolant:
        text(infoTitle_,"COOLANT");
        if(s.coolantTemperatureC.available()) std::snprintf(b,sizeof b,"%.0f°C",s.coolantTemperatureC.value);
        else std::snprintf(b,sizeof b,"—°C");
        break;
    case InfoPage::Battery:
        text(infoTitle_,"BATTERY VOLTAGE");
        if(s.batteryVoltageV.available()) std::snprintf(b,sizeof b,"%.1f V",s.batteryVoltageV.value);
        else std::snprintf(b,sizeof b,"— V");
        break;
    case InfoPage::Tcs:
        text(infoTitle_,"TCS"); std::snprintf(b,sizeof b,"%s",s.tcsEnabled.available()?(s.tcsEnabled.value?"ON":"OFF"):"—"); break;
    case InfoPage::Track:
        text(infoTitle_,"TRACK MODE"); std::snprintf(b,sizeof b,"%s",v.track?"ON":"OFF"); break;
    case InfoPage::Count:text(infoTitle_,"—"); std::snprintf(b,sizeof b,"—"); break;
    }
    text(infoValue_,b);
    const auto fuel=fuelPresentation(s.fuelPercent,now);
    const auto fuelColor=s.fuelPercent.available()&&s.fuelPercent.value<10?p.red:
        s.fuelPercent.available()&&s.fuelPercent.value<=20?p.amber:p.accent;
    const bool lit=fuel.lit;
    if(s.fuelPercent.available()) std::snprintf(b,sizeof b,"%d%%",s.fuelPercent.value); else std::snprintf(b,sizeof b,"—"); text(fuelValue_,b); ink(fuelValue_,fuelColor);
    const int fy=Metrics::fuelY(v.track);
    lv_obj_set_y(fuelTitle_,titleY); lv_obj_set_y(fuelValue_,titleY-5);
    lv_obj_set_y(pump_,fy-10); lv_obj_set_style_line_color(pump_,color(fuelColor),0); show(pump_,lit);
    const int segments=fuel.segments;
    for(int i=0;i<8;++i) { lv_obj_set_y(fuel_[i],fy); lv_obj_set_style_bg_color(fuel_[i],color(i<segments&&lit?fuelColor:p.panel),0); }
    lv_obj_set_y(fuelDivider_,titleY); lv_obj_set_height(fuelDivider_,55); paint(fuelDivider_,p.line);
    const auto phase=life.phase();
    const bool animation=phase==Phase::Boot||phase==Phase::Shutdown;
    const bool cover=phase!=Phase::Running&&!(animation&&urgent(s));
    show(overlay_,cover); paint(overlay_,p.background);
    text(animationTitle_,phase==Phase::Boot?"R15":phase==Phase::Shutdown?"R15":"DISPLAY OFF");
    text(animationSubtitle_,phase==Phase::Boot?"SMART CLUSTER  /  STARTING":phase==Phase::Shutdown?"ENDING THE RIDE":"PRESS O TO TURN THE KEY ON");
    ink(animationSubtitle_,p.muted); paint(animationBed_,p.panel); paint(animationFill_,p.accent);
    show(animationBed_,animation); lv_obj_set_width(animationFill_,std::max(1,int(std::min(life.elapsed(now),Lifecycle::animationMs)*320/Lifecycle::animationMs)));
}
}
