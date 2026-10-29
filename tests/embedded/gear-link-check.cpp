#include "shared/protocol/GearPacket.h"
#include "simulator/lvgl/VehicleSerial.h"
#include <cassert>
#include "shared/protocol/Button.h"
#include "embedded/application/DashboardInput.h"
#include "shared/protocol/DiscreteInputs.h"
#include "firmware/vehicle/main/InputConfig.h"
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
using namespace cluster;
using namespace cluster::gearlink;
static bool feed(Receiver& r,const Bytes& bytes,std::uint64_t now) {
    bool accepted=false;for(auto b:bytes)accepted=r.push(b,now)||accepted;return accepted;
}
int main() {
    inputs::Button button;
    assert(!button.update(true,0));assert(!button.update(true,40)); // held at boot
    assert(!button.update(false,60));assert(!button.update(false,100));
    assert(!button.update(true,120));assert(!button.update(false,140)); // bounce
    assert(!button.update(true,150));assert(!button.update(true,189));
    assert(button.update(true,190));assert(!button.update(true,1000)); // hold
    assert(!button.update(false,1020));assert(!button.update(false,1060));
    assert(!button.update(true,1080));assert(button.update(true,1120));
    Receiver clicks;Packet keys;keys.session=77;keys.sequence=1;keys.uptimeMs=1;
    keys.tripPresses=0xffffffffu;assert(feed(clicks,encode(keys),0));
    assert(clicks.takeClicks().trip==0);
    ++keys.sequence;++keys.uptimeMs;++keys.tripPresses;++keys.infoPresses;
    assert(feed(clicks,encode(keys),20));auto pressed=clicks.takeClicks();
    assert(pressed.trip==1&&pressed.info==1);assert(clicks.takeClicks().trip==0);
    assert(!feed(clicks,encode(keys),30));assert(clicks.takeClicks().info==0);
    DisplayOptions options;applyAction(options,DashboardAction::NextTrip);
    applyAction(options,DashboardAction::NextInfo);
    assert(options.tripPage==TripPage::Trip1&&options.infoPage==InfoPage::Track);
    ++keys.sequence;++keys.uptimeMs;++keys.tripPresses;
    assert(feed(clicks,encode(keys),1020));assert(clicks.takeClicks().trip==0); // stale
    ++keys.session;++keys.sequence;++keys.uptimeMs;++keys.infoPresses;
    assert(feed(clicks,encode(keys),1040));assert(clicks.takeClicks().info==0); // reboot
    static_assert(wiring::valid());
    assert(wiring::left.pin==4&&wiring::right.pin==5&&wiring::highBeam.pin==7);
    assert(wiring::gears[0].pin==6);
    for(int g=1;g<7;++g)assert(wiring::gears[g].pin==7+g);

    for(int g=0;g<7;++g) {
        std::array<bool,7> pins{};pins[g]=true;
        assert(inputs::gear(pins).available()&&inputs::gear(pins).value==g);
        pins[(g+1)%7]=true;assert(inputs::gear(pins).quality==Quality::Invalid);
        Packet sample;sample.gear={g,Quality::Available};sample.left={true,Quality::Available};
        sample.right={false,Quality::Available};sample.highBeam={true,Quality::Available};
        Packet result;assert(decode(encode(sample),result)&&result.gear.value==g);
        assert(result.left.value&&!result.right.value&&result.highBeam.value);
        Receiver lamp;feed(lamp,encode(sample),0);
        assert(lamp.state(0).left.available());assert(lamp.state(1000).left.quality==Quality::Stale);
    }
    assert(!inputs::gear({}).available());
    const char* vector="123456789";
    assert(crc(reinterpret_cast<const std::uint8_t*>(vector),9)==0xcbf43926u);
    Packet p;p.session=123;p.sequence=0xffffffffu;p.uptimeMs=0xfffffff0u;p.levels=1;p.gear=Signal<int>{2,Quality::Available};
    auto b=encode(p);Packet decoded;
    assert(decode(b,decoded)&&decoded.session==123&&decoded.gear.value==2);
    for(std::size_t split=0;split<=packetSize;++split) {
        Receiver r;for(std::size_t i=0;i<split;++i)r.push(b[i],0);
        for(std::size_t i=split;i<packetSize;++i)r.push(b[i],1);
        assert(r.gear(2).value==2);
    }
    Receiver r;assert(!r.gear(0).available());
    assert(!r.received()&&!r.fresh(0));
    for(int i=0;i<50;++i)r.push('x',0);
    assert(feed(r,b,10));assert(r.gear(1009).available());
    assert(r.received()&&r.fresh(1009)&&!r.fresh(1010)&&!r.fresh(9));
    assert(!feed(r,b,500));assert(r.gear(1010).quality==Quality::Stale);
    ++p.sequence;p.uptimeMs=100;p.gear=Signal<int>{3,Quality::Available};
    assert(feed(r,encode(p),1100));assert(r.gear(1100).value==3);
    --p.sequence;assert(!feed(r,encode(p),1200));
    ++p.session;p.sequence=0;p.uptimeMs=1;p.gear={};
    assert(feed(r,encode(p),1300));assert(!r.gear(1300).available());
    for(std::size_t i=0;i<packetSize;++i) {
        auto broken=b;broken[i]^=0x80;assert(!feed(r,broken,1400));
    }
    auto bad=b;bad[4]=4;put(bad,32,crc(bad.data(),32));assert(!decode(bad,decoded));
    bad=b;bad[6]=7;put(bad,32,crc(bad.data(),32));assert(!decode(bad,decoded));
    for(int offset:{20,21,22,23}) {
        bad=b;bad[offset]=3;put(bad,32,crc(bad.data(),32));assert(!decode(bad,decoded));
    }
    assert(feed(r,b,1500));r.reset();assert(!r.gear(1501).available());
    assert(!r.received()&&!r.fresh(1501));
    // Actual POSIX serial adapter against a pseudo-terminal, without hardware.
    const int master=posix_openpt(O_RDWR|O_NOCTTY|O_NONBLOCK);
    assert(master>=0&&grantpt(master)==0&&unlockpt(master)==0);
    {
        host::VehicleSerial serial(ptsname(master));
        assert(!serial.update(0).available());
        assert(write(master,b.data(),b.size())==ssize_t(b.size()));
        Signal<int> observed;
        for(int attempt=0;attempt<50;++attempt){observed=serial.update(100);if(observed.available())break;usleep(1000);}
        assert(observed.available()&&observed.value==2);
        for(int gear=0;gear<=6;++gear) {
            p.session=456;p.sequence=gear;p.uptimeMs=20u*(gear+1);
            p.tripPresses=gear;p.infoPresses=gear;
            p.gear={gear,Quality::Available};p.left={gear%2==0,Quality::Available};
            p.right={gear%2!=0,Quality::Available};p.highBeam={gear==6,Quality::Available};
            const auto frame=encode(p);
            assert(write(master,frame.data(),frame.size())==ssize_t(frame.size()));
            ClusterState state;
            for(int attempt=0;attempt<50;++attempt) {
                state=serial.updateState(200+gear*20);
                if(state.gear.available()&&state.gear.value==gear)break;
                usleep(1000);
            }
            const auto actions=serial.takeClicks();
            assert(actions.trip==unsigned(gear>0)&&actions.info==unsigned(gear>0));
            assert(state.gear.available()&&state.gear.value==gear&&state.turnSignalsArePhase);
            assert(state.left.available()&&state.left.value==p.left.value);
            assert(state.right.available()&&state.right.value==p.right.value);
            assert(state.highBeam.available()&&state.highBeam.value==p.highBeam.value);
        }
        assert(serial.updateState(1320).left.quality==Quality::Stale);
        assert(serial.update(1320).quality==Quality::Stale);
        close(master);
        assert(!serial.update(1400).available());
    }
}
