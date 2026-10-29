#pragma once
#include <cstdint>
#include <cstddef>

namespace cluster {
enum class DateFormat:std::uint8_t { DayMonthYear, MonthDayYear, YearMonthDay };
struct ClockPreferences {
    bool twelveHour=false,automaticTimezone=true;
    DateFormat dateFormat=DateFormat::DayMonthYear;
    std::int16_t utcOffsetMinutes=330;
};
inline bool operator==(const ClockPreferences& a,const ClockPreferences& b) {
    return a.twelveHour==b.twelveHour&&a.automaticTimezone==b.automaticTimezone&&
        a.dateFormat==b.dateFormat&&a.utcOffsetMinutes==b.utcOffsetMinutes;
}
inline bool validClockPreferences(const ClockPreferences& p) {
    return unsigned(p.dateFormat)<=2&&p.utcOffsetMinutes>=-720&&p.utcOffsetMinutes<=840;
}
class Clock {
public:
    static constexpr std::uint64_t firstEpoch=1577836800,lastEpoch=4102444799;
    bool synchronize(std::uint64_t utc,int offset,std::uint64_t now) {
        if(utc<firstEpoch||utc>lastEpoch||offset< -720||offset>840)return false;
        epoch_=utc;offset_=offset;receipt_=now;valid_=true;return true;
    }
    bool available(std::uint64_t now)const {
        return valid_&&now>=receipt_&&epoch_+(now-receipt_)/1000<=lastEpoch;
    }
    std::uint64_t epoch(std::uint64_t now)const{return available(now)?epoch_+(now-receipt_)/1000:0;}
    int offset()const{return offset_;}
private:
    std::uint64_t epoch_=0,receipt_=0;int offset_=0;bool valid_=false;
};
struct ClockSync {std::uint64_t epoch=0;int offset=0;};
// Bounded USB console framing. A consumed malformed frame cannot become UI keys.
class ClockConsole {
public:
    bool consume(char byte,ClockSync& result,bool& ready) {
        ready=false;
        if(!active_) {if(byte!='@')return false;active_=true;size_=0;overflow_=false;return true;}
        if(byte=='\r')return true;
        if(byte!='\n') {if(size_<sizeof buffer_)buffer_[size_++]=byte;else overflow_=true;return true;}
        active_=false;
        if(overflow_||size_<9)return true;
        constexpr char prefix[]="TIME ";
        for(unsigned i=0;i<5;++i)if(buffer_[i]!=prefix[i])return true;
        std::size_t i=5;std::uint64_t utc=0;unsigned digits=0;
        while(i<size_&&buffer_[i]>='0'&&buffer_[i]<='9') {
            if(++digits>10)return true;
            utc=utc*10+unsigned(buffer_[i++]-'0');
        }
        if(!digits||i==size_||buffer_[i++]!=' ')return true;
        int sign=1,offset=0;if(i<size_&&buffer_[i]=='-'){sign=-1;++i;}
        digits=0;
        while(i<size_&&buffer_[i]>='0'&&buffer_[i]<='9') {
            if(++digits>4)return true;
            offset=offset*10+buffer_[i++]-'0';
        }
        offset*=sign;
        if(!digits||i!=size_||utc<Clock::firstEpoch||utc>Clock::lastEpoch||offset< -720||offset>840)return true;
        result={utc,offset};ready=true;return true;
    }
private:
    char buffer_[40]{};std::size_t size_=0;bool active_=false,overflow_=false;
};
}
