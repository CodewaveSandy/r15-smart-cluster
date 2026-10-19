#pragma once
#include <array>
#include <cstdint>
namespace cluster {
// Display-owned confirmed choices. Navigation and vehicle state never persist here.
enum class DashboardDesign : std::uint8_t { Instrument=0 };
struct Preferences { bool light=false; DashboardDesign design=DashboardDesign::Instrument; };
using PreferenceBytes=std::array<std::uint8_t,8>;
inline PreferenceBytes encodePreferences(const Preferences& value) {
    PreferenceBytes bytes{{'R','1','5','P',1,std::uint8_t(value.light),std::uint8_t(value.design),0}};
    for(unsigned i=0;i<7;++i) bytes[7]^=bytes[i];
    return bytes;
}
inline bool decodePreferences(const PreferenceBytes& bytes,Preferences& value) {
    std::uint8_t checksum=0;
    for(unsigned i=0;i<7;++i) checksum^=bytes[i];
    if(bytes[0]!='R'||bytes[1]!='1'||bytes[2]!='5'||bytes[3]!='P'||bytes[4]!=1||bytes[5]>1||bytes[6]!=0||bytes[7]!=checksum) return false;
    value={bool(bytes[5]),DashboardDesign::Instrument}; return true;
}
}
