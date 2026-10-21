#pragma once
// Host-only, bounded nonblocking IPC. The TLS bridge owns phone networking.
#include <array>
#include "shared/model/PhoneState.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace cluster::host {
class PhoneBridge {
    int fd_=-1;
    cluster::PhoneState phone_;
    const char* path_=nullptr;
    bool connected_=false, fresh_=false;
    std::uint64_t last_=0;
public:
    explicit PhoneBridge(const char* path):path_(path) {
        if(!path) return;
        sockaddr_un address{};
        address.sun_family=AF_UNIX;
        if(std::strlen(path)>=sizeof address.sun_path) throw std::runtime_error("Phone bridge IPC path too long");
        std::strcpy(address.sun_path,path);
        fd_=socket(AF_UNIX,SOCK_DGRAM,0);
        if(fd_<0) throw std::runtime_error("Phone bridge socket failed");
        if(fcntl(fd_,F_SETFL,O_NONBLOCK)<0 || bind(fd_,reinterpret_cast<sockaddr*>(&address),sizeof address)<0) {
            close(fd_);fd_=-1;throw std::runtime_error("Phone bridge IPC bind failed");
        }
    }
    PhoneBridge(const PhoneBridge&)=delete;
    PhoneBridge& operator=(const PhoneBridge&)=delete;
    ~PhoneBridge(){if(fd_>=0){close(fd_);unlink(path_);}}
    bool enabled()const{return fd_>=0;}
    bool fresh()const{return fresh_;}
    cluster::PhoneState phone()const{auto state=phone_;state.quality=fresh_?cluster::Quality::Available:cluster::Quality::Unavailable;return state;}
    bool command(char action){
        if(fd_<0||!path_||(action!='P'&&action!='A'&&action!='R'&&action!='D'&&action!='F'))return false;
        std::string destination(path_);
        destination=destination.substr(0,destination.find_last_of('/')+1)+"control.sock";
        sockaddr_un address{};address.sun_family=AF_UNIX;
        if(destination.size()>=sizeof address.sun_path)return false;
        std::strcpy(address.sun_path,destination.c_str());
        return sendto(fd_,&action,1,MSG_DONTWAIT,reinterpret_cast<sockaddr*>(&address),sizeof address)==1;
    }
    bool update(std::uint64_t now) {
        if(fd_<0) return false;
        // Fixed work per UI cycle; reject oversized/truncated/unknown datagrams.
        for(int i=0;i<8;++i){
            std::array<char,32> bytes{};
            const auto count=recv(fd_,bytes.data(),bytes.size(),MSG_DONTWAIT);
            if(count<0)break;
            if(count!=15&&count!=22)continue;
            std::uint64_t sourceMs=0;
            for(unsigned byte=0;byte<8;++byte) sourceMs|=std::uint64_t(static_cast<unsigned char>(bytes[7+byte]))<<(8*byte);
            timespec stamp{};
            if(clock_gettime(CLOCK_MONOTONIC,&stamp)!=0)continue;
            const auto clockMs=std::uint64_t(stamp.tv_sec)*1000+std::uint64_t(stamp.tv_nsec)/1000000;
            if(sourceMs>clockMs||clockMs-sourceMs>=1500)continue;
            if(count==22){
                if(std::memcmp(bytes.data(),"R15D2:",6)!=0||(bytes[6]!='C'&&bytes[6]!='D')||(static_cast<unsigned char>(bytes[15])>7))continue;
                bool codeValid=true;for(unsigned i=16;i<22;++i)if(bytes[i]<'0'||bytes[i]>'9')codeValid=false;
                if(!codeValid)continue;
                auto candidate=phone_;candidate.quality=cluster::Quality::Available;
                candidate.paired=(bytes[15]&1)!=0;candidate.pairingRequested=(bytes[15]&2)!=0;candidate.pairingApproved=(bytes[15]&4)!=0;candidate.connected=bytes[6]=='C';
                for(unsigned i=0;i<6;++i)candidate.pairingCode[i]=bytes[16+i];
                if(!cluster::valid(candidate))continue;
                phone_=candidate;connected_=phone_.connected;fresh_=true;last_=now;
                continue;
            }
            if(std::memcmp(bytes.data(),"R15D1:C",7)==0){connected_=true;fresh_=true;last_=now;}
            else if(std::memcmp(bytes.data(),"R15D1:D",7)==0){connected_=false;fresh_=true;last_=now;}
        }
        if(now-last_>=1500){connected_=false;fresh_=false;}
        return connected_;
    }
};
}
