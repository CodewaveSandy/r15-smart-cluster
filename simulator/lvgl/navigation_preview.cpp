// Manual development renderer of the real v2 store, credit bridge and escaped
// bidirectional tunnel. Simulated phone observations only; no board endpoint.
#include "shared/application/NavigationStore.h"
#include "shared/application/NavigationBridge.h"
#include "shared/protocol/MapTunnel.h"
#include "embedded/ui/Dashboard.h"
#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
namespace {
using namespace cluster;
std::array<std::uint16_t,800*480> pixels{};unsigned warnings=0;
void flush(lv_display_t* d,const lv_area_t* a,std::uint8_t* b){const auto* p=reinterpret_cast<const std::uint16_t*>(b);for(int y=a->y1;y<=a->y2;++y)std::copy_n(p+(y-a->y1)*(a->x2-a->x1+1),a->x2-a->x1+1,pixels.data()+y*800+a->x1);lv_display_flush_ready(d);}
void log(lv_log_level_t,const char* message){++warnings;std::cerr<<message;}
struct Asset {unsigned id;std::vector<std::uint8_t> bytes;navlink::Object object;};
struct Feed {
    navlink::NavigationStore store;navlink::NavigationBridge bridge;
    std::unique_ptr<std::uint8_t[]> raw=std::make_unique<std::uint8_t[]>((navlink::cacheSlots+1)*navlink::maxObject);
    std::unique_ptr<maps::Scene> a=std::make_unique<maps::Scene>(),b=std::make_unique<maps::Scene>();
    maplink::Demux down,up;maplink::Assembler downAssembly,upAssembly;
    std::uint16_t downSeq=0,upSeq=0;std::uint32_t seq=0,locSeq=0,progressSeq=0,session=7,connection=2;
    std::uint64_t now=3000;std::vector<Asset> assets;navlink::Digest source{};unsigned attribution=1;std::int32_t lat=0,lon=0;
    explicit Feed(const std::filesystem::path& directory){
        if(std::filesystem::file_size(directory/"preview.index")>65536)throw std::runtime_error("Oversized preview index");
        std::ifstream index(directory/"preview.index");std::string magic,hash;index>>magic>>hash>>attribution;
        if(magic!="R15NAV2"||hash.size()!=64||attribution<1||attribution>2)throw std::runtime_error("Invalid navigation preview index");
        for(unsigned i=0;i<32;++i){std::size_t used=0;const auto value=std::stoul(hash.substr(i*2,2),&used,16);if(used!=2)throw std::runtime_error("Invalid source digest");source[i]=std::uint8_t(value);}
        unsigned id=0;std::string file;
        while(index>>id>>file){
            if(!id||std::filesystem::path(file).filename()!=file||assets.size()>=128)throw std::runtime_error("Invalid fixture filename/id/count");
            std::ifstream in(directory/file,std::ios::binary);Asset asset;asset.id=id;asset.bytes.resize(navlink::maxObject+1);in.read(reinterpret_cast<char*>(asset.bytes.data()),asset.bytes.size());asset.bytes.resize(std::size_t(in.gcount()));
            if(!navlink::objectHeader(asset.bytes.data(),unsigned(asset.bytes.size()),asset.object))throw std::runtime_error("Invalid fixture object");
            assets.push_back(std::move(asset));
        }
        if(assets.empty())throw std::runtime_error("No complete objects");
        for(const auto& s:assets)if(s.object.kind==navlink::Kind::Overview){lat=navlink::signed32(s.bytes.data(),64);lon=navlink::signed32(s.bytes.data(),68);break;}
        if(!lat&&!lon){const auto& c=assets.front().object;const auto p=maps::tilePoint(c.z,c.x,c.y,2048,2048);lat=maps::latitudeE7(p.north);lon=maps::longitudeE7(p.east);}
        store.initialize(3,raw.get(),a.get(),b.get());store.session(1,2,true);bridge.initialize(1);announce();
        std::cout<<"V2 arena bytes="<<(navlink::cacheSlots+1)*navlink::maxObject+2*maplink::maxPackage+2*sizeof(maps::Scene)<<" scene="<<sizeof(maps::Scene)<<"; explicit HOST SIMULATION, no radio/board/GPS\n";
    }
    void uplink(const maplink::Message& m){
        upSeq=std::uint16_t(upSeq+2);
        for(unsigned off=0;off<m.size;off+=48){const auto record=maplink::fragment(m,2,upSeq,off);for(unsigned i=0;i<record.size;++i)up.push(record.bytes[i],now,[](const std::uint8_t*,unsigned,std::uint64_t){},[&](const std::uint8_t* p,unsigned n,std::uint64_t at){maplink::Message out;if(upAssembly.push(p,n,at,2,out))bridge.downstream(out,at,connection);});}
    }
    void announce(){const auto cap=store.capability();maplink::Message m;m.size=64;std::copy(cap.begin(),cap.end(),m.bytes.begin());uplink(m);}
    void poll(){store.poll(now);uplink(store.status(now));if(store.takeInterest())uplink(store.interest());if(store.takeRouteNeed())uplink(store.routeNeed());announce();}
    maplink::Message packet(navlink::Op op,const std::vector<std::uint8_t>& body={},unsigned object=0,unsigned offset=0){
        const bool loc=op==navlink::Op::Location,progress=op==navlink::Op::Progress;auto& sequence=loc?locSeq:progress?progressSeq:seq;
        if(!++sequence)++sequence;
        auto m=navlink::message(unsigned(op),false,1,connection,3,session,sequence,object,offset,unsigned(body.size()));std::copy(body.begin(),body.end(),m.bytes.begin()+40);navlink::seal(m.bytes.data(),m.size);return m;
    }
    void drain(bool observation=false){
        maplink::Message m;
        while(bridge.take(m,observation)){
            downSeq=std::uint16_t(downSeq+2);if(observation)downSeq|=1;else downSeq&=~1;
            for(unsigned off=0;off<m.size;off+=48){const auto record=maplink::fragment(m,1,downSeq,off);for(unsigned i=0;i<record.size;++i)down.push(record.bytes[i],now,[](const std::uint8_t*,unsigned,std::uint64_t){},[&](const std::uint8_t* p,unsigned n,std::uint64_t at){maplink::Message out;if(downAssembly.push(p,n,at,1,out))store.command(out,at);});}
        }poll();
    }
    void command(navlink::Op op,const std::vector<std::uint8_t>& body={},unsigned object=0,unsigned offset=0){
        auto m=packet(op,body,object,offset);if(!bridge.admit(m,now,connection,true))throw std::runtime_error("Bridge admission unavailable for preview command "+std::to_string(unsigned(op)));
        drain(op==navlink::Op::Location||op==navlink::Op::Progress);
    }
    void open(){std::vector<std::uint8_t> p(40);std::copy(source.begin(),source.end(),p.begin());navlink::put(p.data(),32,1,2);navlink::put(p.data(),34,attribution,2);p[36]=4;command(navlink::Op::Open,p);}
    void fix(unsigned accuracy=500,bool coarse=false){
        now+=501;announce();const auto clock=100000+now;std::vector<std::uint8_t> p(8);navlink::put(p.data(),0,clock,8);command(navlink::Op::TimeRequest,p);const auto request=seq;
        p.assign(20,0);navlink::put(p.data(),0,request);navlink::put(p.data(),4,clock,8);navlink::put(p.data(),12,clock+2,8);command(navlink::Op::TimeConfirm,p);
        p.assign(40,0);navlink::put(p.data(),0,clock+2,8);navlink::put(p.data(),8,clock+2,8);navlink::put(p.data(),16,std::uint32_t(lat));navlink::put(p.data(),20,std::uint32_t(lon));navlink::put(p.data(),24,accuracy);navlink::put(p.data(),28,65535,2);navlink::put(p.data(),30,65535,2);navlink::put(p.data(),32,65535,2);p[34]=coarse?2:1;p[35]=1;command(navlink::Op::Location,p);
    }
    void begin(const Asset& asset,bool badHash=false){std::vector<std::uint8_t> p(40);navlink::put(p.data(),0,asset.bytes.size());maplink::Sha256 hash;hash.update(asset.bytes.data(),asset.bytes.size());auto digest=hash.finish();if(badHash)digest[0]^=1;std::copy(digest.begin(),digest.end(),p.begin()+4);p[36]=std::uint8_t(asset.object.kind);command(navlink::Op::Begin,p,asset.id);}
    void data(const Asset& asset,unsigned until=UINT32_MAX){
        unsigned offset=store.accepted();until=std::min(until,unsigned(asset.bytes.size()));
        while(offset<until){
            for(unsigned i=0;i<4&&offset<until;++i){const unsigned n=std::min(196u,until-offset);std::vector<std::uint8_t> p(asset.bytes.begin()+offset,asset.bytes.begin()+offset+n);auto m=packet(navlink::Op::Data,p,asset.id,offset);if(!bridge.admit(m,now,connection,true))throw std::runtime_error("Preview pipeline lacks downstream credit");offset+=n;}
            drain();++now;
        }
    }
    void commit(const Asset& asset){command(navlink::Op::Commit,{},asset.id);for(unsigned i=0;i<200&&store.stage()==navlink::Stage::Verifying;++i){++now;poll();}for(unsigned i=0;i<200;++i)store.poll(now);}
    void object(const Asset& asset){begin(asset);if(store.stage()!=navlink::Stage::Have){data(asset);commit(asset);}}
    unsigned currentCell()const{
        const auto p=maps::mercator(lat,lon);for(const auto& a:assets)if(a.object.kind==navlink::Kind::Cell){const auto c=maps::tilePoint(a.object.z,a.object.x,a.object.y,2048,2048);const double h=maps::worldMetres/(1u<<a.object.z)/2;if(std::abs(maps::wrapEast(p.east-c.east))<=h&&std::abs(p.north-c.north)<=h)return a.id;}return 0;
    }
};
}
int navigationPreview(const char* path,const char* outputPath){try{
    using namespace cluster;Feed feed(path);const std::filesystem::path output(outputPath);std::filesystem::create_directories(output);
    if(SDL_Init(SDL_INIT_VIDEO))throw std::runtime_error(SDL_GetError());lv_init();lv_log_register_print_cb(log);auto* display=lv_display_create(800,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(display,flush);
    static std::array<std::uint16_t,800*48> buffer;lv_display_set_buffers(display,buffer.data(),nullptr,buffer.size()*2,LV_DISPLAY_RENDER_MODE_PARTIAL);
    ui::Dashboard dashboard(lv_screen_active());ui::ViewOptions view;view.settings=view.map=view.simulated=true;view.time="—:—";view.date="V2 HOST SIMULATION";view.mapZoom=3;
    Lifecycle life;life.key(true,0);life.advance(3000);ClusterState vehicle;
    const auto capture=[&](const char* name){
        ui::MapPageState page;page.navigation=true;page.scene=feed.store.scene();page.pose=feed.store.pose(feed.now);page.transfer="SIMULATED v2 · not live GPS";
        char quality[64],guide[120];
        if(feed.store.accuracyCm()==UINT32_MAX)std::snprintf(quality,sizeof quality,"Source %u · accuracy unknown",unsigned(feed.store.fixState(feed.now)));
        else std::snprintf(quality,sizeof quality,"Source %u · accuracy %.0f m",unsigned(feed.store.fixState(feed.now)),feed.store.accuracyCm()/100.0);
        feed.store.guidanceText(guide,sizeof guide,feed.now);page.location=quality;page.guidance=guide;
        const char* coverage[]={"Missing map detail","Partial cached coverage","Viewport cached"};page.coverage=coverage[unsigned(feed.store.coverage())];
        for(unsigned light=0;light<2;++light)for(unsigned track=0;track<2;++track){view.light=light;view.track=track;dashboard.mapState(page);dashboard.update(vehicle,view,life,feed.now);dashboard.update(vehicle,view,life,feed.now+300);feed.store.camera(view.mapCamera,feed.now);lv_refr_now(nullptr);
            auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),800,480,16,1600,SDL_PIXELFORMAT_RGB565);const auto file=output/(std::string(name)+(track?"-track":"-street")+(light?"-day":"-night")+".bmp");if(!surface||SDL_SaveBMP(surface,file.string().c_str()))throw std::runtime_error(SDL_GetError());SDL_FreeSurface(surface);}
        const auto m=feed.store.metrics();std::cout<<name<<" stage="<<unsigned(feed.store.stage())<<" error="<<unsigned(feed.store.error())<<" accepted="<<feed.store.accepted()<<"/"<<feed.store.total()<<" fix="<<unsigned(feed.store.fixState(feed.now))<<" coverage="<<unsigned(feed.store.coverage())<<" verified="<<m.verified<<" bytes="<<m.bytes<<" credits="<<feed.bridge.credits()<<"\n";
    };
    feed.open();capture("empty");feed.fix();for(unsigned i=0;i<40;++i)feed.poll();capture("position-without-roads");
    const auto current=feed.currentCell();
    for(const auto& a:feed.assets)if(a.object.kind==navlink::Kind::Cell&&a.id==current){feed.begin(a);feed.data(a,unsigned(a.bytes.size()/2));capture("partial-cell-hidden");feed.data(a);feed.commit(a);capture("first-complete-cell");}
    for(const auto& a:feed.assets)if(a.object.kind==navlink::Kind::Cell&&a.id!=current){feed.object(a);if(feed.store.error()==navlink::Error::Resource)break;}
    feed.fix();capture("working-set");
    const auto savedLat=feed.lat,savedLon=feed.lon;feed.lat+=5000000;feed.lon+=5000000;feed.fix();capture("position-outside-old-city");for(unsigned i=0;i<200;++i)feed.poll();capture("missing-roads-position-retained");
    feed.lat=savedLat;feed.lon=savedLon;feed.fix();capture("return-to-cached-area");for(unsigned i=0;i<200;++i)feed.poll();capture("cached-area-restored");
    dashboard.mapPointer(view,life,{maps::TouchPhase::Press,530,300});dashboard.mapPointer(view,life,{maps::TouchPhase::Move,650,340});dashboard.mapPointer(view,life,{maps::TouchPhase::Release,650,340});capture("browse-with-position");
    feed.lon+=1000;feed.fix();capture("browse-new-fix-camera-retained");feed.lon=savedLon;
    feed.fix(15000,true);capture("approximate-position");feed.now+=2100;feed.poll();capture("last-known-position");feed.now+=8000;feed.poll();capture("expired-position-hidden");feed.fix();
    feed.now+=501;feed.poll();{
        std::vector<std::uint8_t> p(40);navlink::put(p.data(),0,100000+feed.now+1000,8);navlink::put(p.data(),8,100000+feed.now+1000,8);navlink::put(p.data(),16,std::uint32_t(feed.lat));navlink::put(p.data(),20,std::uint32_t(feed.lon));navlink::put(p.data(),24,500);navlink::put(p.data(),28,65535,2);navlink::put(p.data(),30,65535,2);navlink::put(p.data(),32,65535,2);p[34]=p[35]=1;feed.command(navlink::Op::Location,p);
    }capture("future-source-rejected");feed.now+=1100;feed.poll();capture("future-never-becomes-live");feed.fix();
    unsigned overview=0,window=0;std::uint64_t route=0;unsigned revision=0,distance=0;
    for(const auto& a:feed.assets)if(a.object.kind!=navlink::Kind::Cell){feed.object(a);route=a.object.route;revision=a.object.revision;distance=a.object.distance;if(a.object.kind==navlink::Kind::Overview)overview=a.id;else window=a.id;}
    if(overview&&window&&current){
        std::vector<std::uint8_t> p(24);navlink::put(p.data(),0,route,8);navlink::put(p.data(),8,revision);navlink::put(p.data(),12,overview);navlink::put(p.data(),16,window);navlink::put(p.data(),20,current);feed.command(navlink::Op::Activate,p);for(unsigned i=0;i<200;++i)feed.poll();capture("route-context-activated");
        feed.fix();p.assign(40,0);navlink::put(p.data(),0,route,8);navlink::put(p.data(),8,revision);navlink::put(p.data(),12,100000+feed.now+2,8);navlink::put(p.data(),20,distance/2);navlink::put(p.data(),24,120);navlink::put(p.data(),28,std::min(75u,distance/2));p[32]=1;p[33]=1;feed.command(navlink::Op::Progress,p);capture("simulated-route-guidance");
        view.mapCamera.area();capture("route-overview");view.mapCamera.follow();feed.fix(25000,true);capture("approximate-no-guidance");
        feed.fix();
        Asset nextOverview,nextWindow;
        for(const auto& a:feed.assets){if(a.object.kind==navlink::Kind::Overview)nextOverview=a;if(a.object.kind==navlink::Kind::RouteWindow)nextWindow=a;}
        Asset shortWindow=nextWindow;shortWindow.id=1000020;
        const unsigned shortCount=std::max(2u,shortWindow.object.count/2);shortWindow.bytes.resize(64+shortCount*8);
        navlink::put(shortWindow.bytes.data(),30,shortCount,2);navlink::put(shortWindow.bytes.data(),44,std::max(1u,distance/2));navlink::put(shortWindow.bytes.data(),56,gearlink::crc(shortWindow.bytes.data()+64,unsigned(shortWindow.bytes.size()-64)));
        navlink::objectHeader(shortWindow.bytes.data(),unsigned(shortWindow.bytes.size()),shortWindow.object);feed.object(shortWindow);
        p.assign(24,0);navlink::put(p.data(),0,route,8);navlink::put(p.data(),8,revision);navlink::put(p.data(),12,overview);navlink::put(p.data(),16,shortWindow.id);navlink::put(p.data(),20,current);feed.command(navlink::Op::Activate,p);
        feed.fix();p.assign(40,0);navlink::put(p.data(),0,route,8);navlink::put(p.data(),8,revision);navlink::put(p.data(),12,100000+feed.now+2,8);navlink::put(p.data(),20,distance/4);p[32]=3;p[33]=1;feed.command(navlink::Op::Progress,p);capture("next-window-needed-no-guidance");
        maplink::Message request;if(feed.bridge.takeRouteNeed(request))std::cout<<"ROUTE_NEED delivered through tunnel and vehicle bridge, bytes="<<request.size<<"\n";
        nextOverview.id=1000100;nextWindow.id=1000101;
        navlink::put(nextOverview.bytes.data(),24,2);navlink::put(nextWindow.bytes.data(),24,2);
        navlink::objectHeader(nextOverview.bytes.data(),unsigned(nextOverview.bytes.size()),nextOverview.object);navlink::objectHeader(nextWindow.bytes.data(),unsigned(nextWindow.bytes.size()),nextWindow.object);
        feed.object(nextOverview);capture("reroute-staged-old-roads");
        p.assign(24,0);navlink::put(p.data(),0,route,8);navlink::put(p.data(),8,2);navlink::put(p.data(),12,nextOverview.id);navlink::put(p.data(),16,nextWindow.id);navlink::put(p.data(),20,current);feed.command(navlink::Op::Activate,p);capture("reroute-minimum-context-missing");
        feed.object(nextWindow);feed.command(navlink::Op::Activate,p);for(unsigned i=0;i<200;++i)feed.poll();capture("reroute-atomic-activation");
        navlink::put(p.data(),8,revision);navlink::put(p.data(),12,overview);navlink::put(p.data(),16,window);feed.command(navlink::Op::Activate,p);capture("old-route-revision-rejected");
    }
    Asset damaged=feed.assets.front();damaged.id=2000000;feed.begin(damaged,true);feed.data(damaged);feed.commit(damaged);capture("hash-failure-old-cells-retained");
    feed.begin(damaged,true);feed.data(damaged,unsigned(damaged.bytes.size()/3));feed.command(navlink::Op::Cancel,{},damaged.id);capture("cancel-retains-complete-cells");
    feed.begin(damaged,true);feed.data(damaged,unsigned(damaged.bytes.size()/3));feed.now+=15000;feed.poll();capture("timeout-retains-complete-cells");feed.fix();
    vehicle.engineWarning=vehicle.coolantWarning={true,Quality::Available};capture("warnings");vehicle={};feed.command(navlink::Op::Stop);capture("stopped-cached-map");
    feed.command(navlink::Op::End);for(unsigned i=0;i<200;++i)feed.poll();capture("end-after-stop-clears-route");
    view.mapCamera.drag(-160,0);feed.store.camera(view.mapCamera,feed.now);for(unsigned i=0;i<200;++i)feed.store.poll(feed.now);capture("offline-cached-pan");
    feed.store.session(1,0,false);capture("disconnected-cache");
    ++feed.connection;++feed.session;feed.seq=feed.locSeq=feed.progressSeq=0;feed.bridge.invalidate();feed.store.session(1,feed.connection,true);feed.announce();feed.open();for(unsigned i=0;i<200;++i)feed.poll();capture("reconnect-no-source-or-guidance");
    for(const auto& a:feed.assets)if(a.id==current){feed.begin(a);capture("full-sha-reuse-after-reconnect");break;}
    life.key(false,feed.now);capture("shutdown");life.advance(feed.now+2000);feed.now+=2000;capture("off");life=Lifecycle{};life.key(true,feed.now);capture("boot");
    std::cout<<"LVGL warnings="<<warnings<<"; no physical throughput, touch, GPS or radio acceptance claimed\n";return warnings?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
