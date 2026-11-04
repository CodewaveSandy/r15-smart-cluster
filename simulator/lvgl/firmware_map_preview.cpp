// Development preview of the actual portable firmware store and Dashboard page.
// No board/radio backdoor, automated suite or live GPS source.
#include "shared/application/MapStore.h"
#include "shared/protocol/MapTunnel.h"
#include "embedded/ui/Dashboard.h"
#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
namespace {
using namespace cluster;
std::array<std::uint16_t,800*480> pixels{};
unsigned warnings=0;
void flush(lv_display_t* d,const lv_area_t* a,std::uint8_t* b){const auto* p=reinterpret_cast<const std::uint16_t*>(b);for(int y=a->y1;y<=a->y2;++y)std::copy_n(p+(y-a->y1)*(a->x2-a->x1+1),a->x2-a->x1+1,pixels.data()+y*800+a->x1);lv_display_flush_ready(d);}
void log(lv_log_level_t,const char* message){++warnings;std::cerr<<message;}
struct Feed {
    maplink::MapStore store;std::array<std::unique_ptr<std::uint8_t[]>,2> binary;
    std::array<std::unique_ptr<maps::Scene>,2> scenes;
    std::vector<std::uint8_t> package;std::uint64_t now=3000;std::uint32_t sequence=0,generation=1;unsigned sent=0;std::uint64_t phoneClock=100000;std::string title="Share map from phone";
    maplink::Demux demux;maplink::Assembler assembler;std::uint16_t transport=0;
    explicit Feed(const char* path){std::ifstream in(path,std::ios::binary);package.assign(std::istreambuf_iterator<char>(in),{});if(package.size()<64||package.size()>65536)throw std::runtime_error("Invalid package length");
        for(unsigned i=0;i<2;++i){binary[i]=std::make_unique<std::uint8_t[]>(65536);scenes[i]=std::make_unique<maps::Scene>();}store.initialize(3,binary[0].get(),binary[1].get(),scenes[0].get(),scenes[1].get());store.session(1,2,true);
        maplink::Sha256 sha;sha.update(package.data(),package.size());std::cout<<"Package SHA256 ";for(auto v:sha.finish())std::printf("%02x",v);std::cout<<"\n";
    }
    void command(maplink::Type type,const std::vector<std::uint8_t>& body={},bool map=false){
        maplink::Message m;m.size=unsigned(36+body.size());auto* b=m.bytes.data();maplink::prefix(b,'X');b[5]=std::uint8_t(type);b[6]=32;maplink::put(b,8,1);maplink::put(b,12,2);maplink::put(b,16,3);maplink::put(b,20,map?generation:0);maplink::put(b,24,++sequence);maplink::put(b,28,body.size(),2);std::copy(body.begin(),body.end(),b+32);maplink::seal(b,m.size);
        ++transport;
        for(unsigned offset=0;offset<m.size;offset+=48){const auto r=maplink::fragment(m,1,transport,offset);for(unsigned i=0;i<r.size;++i)demux.push(r.bytes[i],now,[](const std::uint8_t*,unsigned,std::uint64_t){},[&](const std::uint8_t* p,unsigned n,std::uint64_t at){maplink::Message out;if(assembler.push(p,n,at,1,out))store.command(out,at);});}
        store.poll(now);
    }
    void begin(){if(store.scene())maplink::put(package.data(),40,maplink::get(package.data(),40)+1);++generation;sent=0;std::vector<std::uint8_t> body(36);maplink::put(body.data(),0,package.size());maplink::Sha256 sha;sha.update(package.data(),package.size());auto digest=sha.finish();std::copy(digest.begin(),digest.end(),body.begin()+4);command(maplink::Type::Begin,body,true);title=store.scene()?"Updating map · 0%":"Loading map · 0%";}
    void data(unsigned until){while(sent<std::min<unsigned>(until,package.size())){const auto n=std::min<unsigned>(200,package.size()-sent);std::vector<std::uint8_t> b(4+n);maplink::put(b.data(),0,sent);std::copy_n(package.data()+sent,n,b.data()+4);command(maplink::Type::Data,b,true);sent+=n;}
        title=(store.scene()?"Updating map · ":"Loading map · ")+std::to_string(sent*100/package.size())+"%";}
    void commit(){command(maplink::Type::Commit,{},true);title="Checking map...";}
    void finish(){for(unsigned i=0;i<300;++i){store.poll(now);const auto s=store.status(now);if(s[6]!=std::uint8_t(maplink::State::Verifying)){title=s[6]==4?"Offline road map":"Map update failed · Retry";break;}}}
    void fix(bool direction=true,bool outside=false){const auto clock=phoneClock++;std::vector<std::uint8_t> b(8);maplink::put(b.data(),0,clock,8);command(maplink::Type::TimeRequest,b);const auto request=sequence;b.assign(20,0);maplink::put(b.data(),0,request);maplink::put(b.data(),4,clock,8);maplink::put(b.data(),12,clock+2,8);command(maplink::Type::TimeConfirm,b);
        b.assign(40,0);maplink::put(b.data(),0,clock+2,8);maplink::put(b.data(),8,clock+2,8);maplink::put(b.data(),16,maplink::get(package.data(),8)+(outside?500000:0));maplink::put(b.data(),20,maplink::get(package.data(),12));maplink::put(b.data(),24,500);maplink::put(b.data(),28,9000,2);maplink::put(b.data(),30,1000,2);maplink::put(b.data(),32,250,2);if(!direction){maplink::put(b.data(),28,65535,2);maplink::put(b.data(),30,65535,2);maplink::put(b.data(),32,65535,2);}b[34]=b[35]=1;command(maplink::Type::Location,b);}
    void accuracyLimited(){std::vector<std::uint8_t> b(40);maplink::put(b.data(),8,phoneClock+2,8);maplink::put(b.data(),24,UINT32_MAX);maplink::put(b.data(),28,65535,2);maplink::put(b.data(),30,65535,2);maplink::put(b.data(),32,65535,2);b[34]=5;b[35]=3;command(maplink::Type::Location,b);}
};
}
int navigationPreview(const char*,const char*);
int main(int argc,char** argv){try{
    if(argc==5&&std::string(argv[1])=="--navigation-bundle"&&std::string(argv[3])=="--render")return navigationPreview(argv[2],argv[4]);
    if(argc!=5||std::string(argv[1])!="--package"||std::string(argv[3])!="--render"){std::cerr<<"Usage: r15-map-firmware-preview --package region.r15b --render directory\nDevelopment simulation only; no live GPS/radio.\n";return 2;}
    Feed feed(argv[2]);std::cout<<"Scene bytes="<<sizeof(maps::Scene)<<"; reserved map bytes="<<2*sizeof(maps::Scene)+2*maplink::maxPackage<<"\n";
    const std::filesystem::path output=argv[4];std::filesystem::create_directories(output);
    if(SDL_Init(SDL_INIT_VIDEO))throw std::runtime_error(SDL_GetError());
    lv_init();lv_log_register_print_cb(log);auto* display=lv_display_create(800,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(display,flush);
    static std::array<std::uint16_t,800*48> buffer;lv_display_set_buffers(display,buffer.data(),nullptr,buffer.size()*2,LV_DISPLAY_RENDER_MODE_PARTIAL);
    ui::Dashboard dashboard(lv_screen_active());ui::ViewOptions view;view.settings=view.map=true;view.simulated=true;view.time="—:—";view.date="MAP TRANSFER SIMULATION";
    Lifecycle life;life.key(true,0);life.advance(3000);ClusterState vehicle;ui::MapPageState page;
    const auto capture=[&](const char* name){
        const auto s=feed.store.status(feed.now);const char* locations[]={"Timing unavailable","Waiting for phone location","SIMULATED fix · not live GPS","Location stale · marker hidden","Location accuracy low","Outside downloaded area","Location sharing stopped","Location invalid"};
        page.scene=feed.store.scene();page.pose=feed.store.pose(feed.now);page.transfer=feed.title.c_str();page.location=locations[s[68]];
        for(unsigned light=0;light<2;++light)for(unsigned track=0;track<2;++track){view.light=light;view.track=track;dashboard.mapState(page);dashboard.update(vehicle,view,life,feed.now);dashboard.update(vehicle,view,life,feed.now+300);lv_refr_now(nullptr);
            auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),800,480,16,1600,SDL_PIXELFORMAT_RGB565);const auto file=output/(std::string(name)+(track?"-track":"-street")+(light?"-day":"-night")+".bmp");if(!surface||SDL_SaveBMP(surface,file.string().c_str()))throw std::runtime_error(SDL_GetError());SDL_FreeSurface(surface);}
        std::cout<<name<<": state="<<unsigned(s[6])<<" error="<<unsigned(s[7])<<" accepted="<<maplink::get(s.data(),28)<<"/"<<maplink::get(s.data(),32)<<" location="<<unsigned(s[68])<<"\n";
    };
    capture("empty");feed.begin();capture("loading-zero");feed.data(unsigned(feed.package.size()/2));capture("loading-half");feed.data(unsigned(feed.package.size()));feed.commit();capture("checking");feed.finish();capture("active-no-fix");feed.fix();capture("simulated-position");feed.fix(false);capture("simulated-neutral-position");
    dashboard.mapPointer(view,life,{maps::TouchPhase::Press,540,300});
    dashboard.mapPointer(view,life,{maps::TouchPhase::Move,680,350});
    dashboard.mapPointer(view,life,{maps::TouchPhase::Release,680,350});
    capture("browse-position");feed.fix(false);capture("browse-new-fix");
    view.mapFocus=4;settingsAction(view,SettingsAction::Select,vehicle,life);capture("browse-zoom");
    view.mapFocus=5;settingsAction(view,SettingsAction::Select,vehicle,life);capture("physical-pan");
    settingsAction(view,SettingsAction::Select,vehicle,life);capture("physical-pan-west");
    settingsAction(view,SettingsAction::Back,vehicle,life);
    view.mapFocus=1;settingsAction(view,SettingsAction::Select,vehicle,life);capture("follow-restored");
    view.mapFocus=2;settingsAction(view,SettingsAction::Select,vehicle,life);capture("area-overview");
    view.mapFocus=1;settingsAction(view,SettingsAction::Select,vehicle,life);
    feed.accuracyLimited();capture("low-accuracy");feed.fix(false,true);capture("outside-coverage");feed.fix();feed.begin();feed.data(unsigned(feed.package.size()/2));capture("replacement-half");feed.command(maplink::Type::Cancel,{},true);feed.title="Map update cancelled";capture("cancelled");
    feed.begin();feed.data(unsigned(feed.package.size()));feed.binary[1-(feed.store.scene()==feed.scenes[0].get()?0:1)][feed.package.size()-1]^=1;feed.commit();feed.finish();capture("hash-failure-old-map");
    feed.now+=2000;feed.store.poll(feed.now);capture("stale");vehicle.engineWarning=vehicle.coolantWarning={true,Quality::Available};capture("warnings");vehicle={};feed.command(maplink::Type::Stop);capture("stopped");feed.store.session(1,0,false);capture("disconnected");feed.store.session(1,2,true);feed.begin();feed.now+=15000;feed.store.poll(feed.now);feed.title="Map transfer timed out";capture("timeout-old-map");
    std::cout<<"LVGL warnings="<<warnings<<"; development preview only, no board/radio acceptance\n";return warnings?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
