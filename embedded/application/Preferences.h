#pragma once
#include "Clock.h"
#include <array>
#include <cstdint>
#include <cstddef>
namespace cluster {
// Display-owned confirmed choices; never persist telemetry or a fabricated clock.
enum class DashboardDesign : std::uint8_t { Instrument=0 };
struct Preferences { bool light=false; DashboardDesign design=DashboardDesign::Instrument; ClockPreferences clock{}; };
inline bool operator==(const Preferences& a,const Preferences& b) {return a.light==b.light&&a.design==b.design&&a.clock==b.clock;}
using PreferenceBytes=std::array<std::uint8_t,16>;
inline PreferenceBytes encodePreferences(const Preferences& value) {
    const auto offset=std::uint16_t(value.clock.utcOffsetMinutes);
    PreferenceBytes bytes{{'R','1','5','P',2,std::uint8_t(value.light),std::uint8_t(value.design),
        std::uint8_t(value.clock.twelveHour),std::uint8_t(value.clock.dateFormat),std::uint8_t(value.clock.automaticTimezone),
        std::uint8_t(offset),std::uint8_t(offset>>8),0,0,0,0}};
    for(unsigned i=0;i<15;++i)bytes[15]^=bytes[i];
    return bytes;
}
inline bool decodePreferences(const std::uint8_t* bytes,std::size_t size,Preferences& value) {
    if((size!=8&&size!=16)||bytes[0]!='R'||bytes[1]!='1'||bytes[2]!='5'||bytes[3]!='P'||bytes[5]>1||bytes[6]!=0)return false;
    std::uint8_t checksum=0;for(std::size_t i=0;i<size-1;++i)checksum^=bytes[i];
    if(checksum!=bytes[size-1])return false;
    Preferences result;result.light=bool(bytes[5]);
    if(size==8) {if(bytes[4]!=1)return false;}
    else {
        if(bytes[4]!=2||bytes[7]>1||bytes[8]>2||bytes[9]>1||bytes[12]||bytes[13]||bytes[14])return false;
        const unsigned raw=unsigned(bytes[10])|(unsigned(bytes[11])<<8);
        result.clock={bool(bytes[7]),bool(bytes[9]),DateFormat(bytes[8]),std::int16_t(raw<32768?int(raw):int(raw)-65536)};
        if(!validClockPreferences(result.clock))return false;
    }
    value=result;return true;
}
template<std::size_t N> inline bool decodePreferences(const std::array<std::uint8_t,N>& bytes,Preferences& value) {return decodePreferences(bytes.data(),bytes.size(),value);}
}
