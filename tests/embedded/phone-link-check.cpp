#include <fstream>
#include <string>
#include "shared/protocol/PhoneLink.h"
#include <cassert>
#include <cstring>
using namespace cluster;using namespace cluster::phonelink;
static bool feed(Receiver& r,const Bytes& b,std::uint64_t now,Kind kind=Kind::Status){bool accepted=false;for(auto byte:b)accepted=r.push(byte,now,kind)||accepted;return accepted;}
int main(int argc,char** argv){
    assert(argc==2);std::ifstream vectors(argv[1]);assert(vectors.good());
    std::string line;unsigned vectorCount=0;
    while(std::getline(vectors,line)) {
        if(line.empty()||line[0]=='#')continue;
        const bool expected=line[0]=='1';const auto hexValue=line.substr(2);assert(hexValue.size()==96);
        Bytes bytes{};for(unsigned i=0;i<packetSize;++i)bytes[i]=std::uint8_t(std::stoul(hexValue.substr(i*2,2),nullptr,16));
        Packet value;assert(decode(bytes,value)==expected);++vectorCount;
    }
    assert(vectorCount>=6);
    Packet sample;sample.vehicle=0x11223344;sample.sequence=1;sample.uptime=20;
    sample.phone.quality=Quality::Available;sample.phone.bleBench=true;sample.phone.pairWindowOpen=true;sample.phone.pairingRequested=true;
    sample.pairing=0x55667788;sample.code=123456;
    const auto encoded=encode(sample);
    const char* golden="5231355001011400443322110000000001000000000000008877665540e201000000000014000000000000007390f5a8";
    const auto hex=[](char c){return unsigned(c>='a'?c-'a'+10:c-'0');};
    for(unsigned i=0;i<packetSize;++i)assert(encoded[i]==(hex(golden[i*2])*16+hex(golden[i*2+1])));
    Packet decoded;assert(decode(encoded,decoded)&&decoded.code==123456&&std::strcmp(decoded.phone.pairingCode.data(),"123456")==0);
    for(unsigned split=0;split<=packetSize;++split){Receiver r;for(unsigned i=0;i<split;++i)r.push(encoded[i],0,Kind::Status);for(unsigned i=split;i<packetSize;++i)r.push(encoded[i],1,Kind::Status);assert(r.fresh(2));}
    Receiver receiver;for(unsigned i=0;i<80;++i)receiver.push('x',0,Kind::Status);
    assert(feed(receiver,encoded,10)&&receiver.fresh(1009)&&!receiver.fresh(1010));
    assert(!feed(receiver,encoded,1000)&&!receiver.fresh(1010));
    for(unsigned i=0;i<packetSize;++i){auto corrupt=encoded;corrupt[i]^=0x80;assert(!feed(receiver,corrupt,1020));}
    for(unsigned i:{4u,5u,6u,7u,32u,33u,34u,40u}) {auto bad=encoded;bad[i]=255;put(bad,44,gearlink::crc(bad.data(),44));assert(!decode(bad,decoded));}
    auto invalid=sample;invalid.phone.connected=true;assert(!decode(encode(invalid),decoded));
    invalid=sample;invalid.code=1000000;assert(!decode(encode(invalid),decoded));
    Packet status;status.vehicle=12;status.phone.quality=Quality::Available;status.phone.bleBench=true;
    Client display;display.initialize(99);display.update(status,100);
    assert(!display.phone(100,12).available());Packet control;
    assert(display.outgoing(100,control)&&control.action==Action::Heartbeat);
    assert(decode(encode(control),decoded)&&decoded.display==99);
    Gate gate(12);Request request;bool changed=false;
    assert(!gate.receive(control,100,0,request,changed)&&changed&&gate.fresh(1099)&&!gate.fresh(1100));
    gate.stamp(status);display.update(status,120);assert(display.phone(120,12).available());
    assert(!display.phone(120,13).available());
    assert(display.post(Action::Pair,120,12));assert(display.outgoing(120,control));
    assert(gate.receive(control,120,0,request,changed)&&request.action==Action::Pair);gate.queued(true);
    assert(!gate.receive(control,140,0,request,changed)); // Same request cannot execute twice.
    gate.stamp(status);display.update(status,160);assert(!display.phone(160,12).commandFailed);
    status.phone.pairWindowOpen=true;status.phone.pairingRequested=true;status.pairing=123;status.code=456;
    display.update(status,200);assert(display.post(Action::Approve,200,12)&&display.outgoing(200,control));
    assert(control.pairing==123);
    assert(!gate.receive(control,200,124,request,changed)); // Old numeric-comparison request rejected.
    gate.stamp(status);assert(status.result==Result::Rejected);display.update(status,220);assert(display.phone(220,12).commandFailed);
    assert(display.post(Action::Reject,240,12)&&display.outgoing(240,control));
    assert(gate.receive(control,240,123,request,changed));gate.queued(false);
    gate.stamp(status);display.update(status,260);assert(display.phone(260,12).commandFailed);
    assert(display.post(Action::Pair,280,12)&&display.outgoing(280,control));
    display.outgoing(1800,control);assert(!display.phone(1800,12).available()); // Link freshness independent of request timeout.
    status.phone={};status.phone.quality=Quality::Available;status.vehicle=13;status.display=0;status.pairing=status.code=0;status.result=Result::None;status.request=0;status.action=Action::Heartbeat;
    display.update(status,1820);assert(!display.phone(1820,13).available());
    assert(display.phone(1820,13).unavailable==PhoneUnavailable::ReturnChannel);
    assert(display.phone(1820,14).unavailable==PhoneUnavailable::VehicleSession);
    assert(display.phone(2820,13).unavailable==PhoneUnavailable::Status); // Requires new display-session echo.
    gate.invalidate();assert(!gate.fresh(1820));
    Packet hello;hello.kind=Kind::Control;hello.vehicle=12;hello.display=99;
    assert(!gate.receive(hello,1830,0,request,changed));
    auto replay=control;replay.vehicle=12;replay.display=99;replay.request=3;replay.action=Action::Reject;replay.pairing=123;
    assert(!gate.receive(replay,1840,123,request,changed)); // Dedupe survives an RX overflow/reset.
    Packet beforeReset;assert(display.outgoing(1840,beforeReset));display.reset();
    display.update(status,1850);Packet afterReset;assert(display.outgoing(1850,afterReset)&&afterReset.sequence>beforeReset.sequence);
    auto wrong=control;wrong.vehicle=11;wrong.action=Action::Pair;wrong.request=10;
    assert(!gate.receive(wrong,1900,0,request,changed));
    Receiver controls;auto heartbeat=Packet{};heartbeat.kind=Kind::Control;heartbeat.vehicle=12;heartbeat.display=99;heartbeat.uptime=0xffffffff;heartbeat.sequence=0xffffffff;
    assert(feed(controls,encode(heartbeat),0,Kind::Control));heartbeat.uptime=1;heartbeat.sequence=1;assert(feed(controls,encode(heartbeat),20,Kind::Control));
    Client forget;forget.initialize(77);Packet saved;saved.vehicle=12;saved.display=77;
    saved.phone.quality=Quality::Available;saved.phone.paired=saved.phone.connected=true;
    forget.update(saved,100);assert(forget.post(Action::Forget,110,12));
    assert(forget.phone(110,12).forgetting);Packet forgettingCommand;assert(forget.outgoing(110,forgettingCommand));
    saved.request=forgettingCommand.request;saved.action=Action::Forget;saved.result=Result::Queued;
    forget.update(saved,200);assert(forget.phone(200,12).forgetting); // Queue ACK is not completion.
    assert(!forget.post(Action::Forget,210,12));
    saved.phone.connected=saved.phone.paired=false;forget.update(saved,300);
    assert(!forget.phone(300,12).forgetting&&!forget.phone(300,12).paired);
    saved.phone.paired=true;forget.update(saved,400);assert(forget.post(Action::Forget,410,12));
    assert(forget.outgoing(410,forgettingCommand));saved.request=forgettingCommand.request;
    forget.update(saved,500);assert(forget.phone(500,12).forgetting);
    forget.update(saved,3800);forget.outgoing(3910,forgettingCommand);
    assert(!forget.phone(3910,12).forgetting&&forget.phone(3910,12).commandFailed);

}
