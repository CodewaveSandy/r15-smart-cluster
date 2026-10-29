#pragma once
#include "shared/protocol/GearPacket.h"
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

namespace cluster::host {
class VehicleSerial {
    int fd_=-1;
    gearlink::Receiver receiver_;
public:
    explicit VehicleSerial(const char* path) {
        if(!path)return;
        fd_=open(path,O_RDONLY|O_NOCTTY|O_NONBLOCK);
        if(fd_<0)throw std::runtime_error("Cannot open vehicle serial port");
        termios options{};
        if(tcgetattr(fd_,&options)!=0){close(fd_);fd_=-1;throw std::runtime_error("Cannot read serial configuration");}
        cfmakeraw(&options);cfsetispeed(&options,B115200);cfsetospeed(&options,B115200);
        options.c_cflag|=CLOCAL|CREAD;options.c_cflag&=~(PARENB|CSTOPB|CSIZE|CRTSCTS);options.c_cflag|=CS8;
        options.c_cc[VMIN]=0;options.c_cc[VTIME]=0;
        if(tcsetattr(fd_,TCSANOW,&options)!=0){close(fd_);fd_=-1;throw std::runtime_error("Cannot configure serial port");}
        tcflush(fd_,TCIFLUSH);
    }
    bool fresh(std::uint64_t now)const{return receiver_.fresh(now);}
    bool received()const{return receiver_.received();}
    gearlink::ButtonClicks takeClicks(){return receiver_.takeClicks();}
    Signal<int> update(std::uint64_t now) {return updateState(now).gear;}
    ~VehicleSerial(){if(fd_>=0)close(fd_);}
    VehicleSerial(const VehicleSerial&)=delete;
    VehicleSerial& operator=(const VehicleSerial&)=delete;
    ClusterState updateState(std::uint64_t now) {
        if(fd_<0)return {};
        std::uint8_t buffer[1024];
        // Bound I/O per UI cycle. Excess backlog is discarded, not replayed.
        for(int batch=0;batch<4;++batch) {
            const auto n=read(fd_,buffer,sizeof buffer);
            if(n<0) {
                if(errno!=EAGAIN&&errno!=EWOULDBLOCK&&errno!=EINTR){close(fd_);fd_=-1;receiver_.reset();}
                break;
            }
            if(n==0)break;
            for(ssize_t i=0;i<n;++i)if(receiver_.push(buffer[i],now)) {
                const auto& p=receiver_.latest();
                if(p.sequence%50==0)std::fprintf(stderr,"Vehicle: GPIO4=%u GPIO5=%u left=%d right=%d highBeam=%d gear=%d quality=%d\n",p.levels&1,(p.levels>>1)&1,int(p.left.value),int(p.right.value),int(p.highBeam.value),p.gear.value,int(p.gear.quality));
            }
            if(batch==3){tcflush(fd_,TCIFLUSH);receiver_.reset();}
        }
        return receiver_.state(now);
    }
};
}
