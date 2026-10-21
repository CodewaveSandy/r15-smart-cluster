#include "simulator/lvgl/PhoneBridge.h"
#include <cassert>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(){
    char directory[]="/tmp/r15-ipc-test-XXXXXX";
    assert(mkdtemp(directory));
    const std::string path=std::string(directory)+"/status.sock";
    {
        cluster::host::PhoneBridge bridge(path.c_str());
        assert(bridge.enabled());
        const int sender=socket(AF_UNIX,SOCK_DGRAM,0);assert(sender>=0);
        sockaddr_un address{};address.sun_family=AF_UNIX;
        std::strcpy(address.sun_path,path.c_str());
        auto send=[&](const char* text,unsigned ageMs=0){
            std::string frame=text;
            timespec stamp{};assert(clock_gettime(CLOCK_MONOTONIC,&stamp)==0);
            const auto sourceMs=std::uint64_t(stamp.tv_sec)*1000+std::uint64_t(stamp.tv_nsec)/1000000-ageMs;
            for(unsigned byte=0;byte<8;++byte)frame.push_back(char((sourceMs>>(8*byte))&255));
            assert(sendto(sender,frame.data(),frame.size(),0,reinterpret_cast<sockaddr*>(&address),sizeof address)>0);
        };
        assert(!bridge.update(0));assert(!bridge.fresh());
        send("R15D1:C");assert(bridge.update(100));
        send("R15D1:Cgarbage");assert(bridge.update(1599));
        assert(!bridge.update(1600));assert(!bridge.fresh());
        send("R15D1:C");assert(bridge.update(1700));
        send("R15D1:D");assert(!bridge.update(1800));assert(bridge.fresh());
        send("bad");assert(!bridge.update(1900));
        assert(!bridge.update(3300));assert(!bridge.fresh());
        send("R15D1:C",2000);assert(!bridge.update(3400));assert(!bridge.fresh());
        close(sender);
    }
    assert(access(path.c_str(),F_OK)!=0);rmdir(directory);
    cluster::host::PhoneBridge disabled(nullptr);assert(!disabled.enabled());
}
