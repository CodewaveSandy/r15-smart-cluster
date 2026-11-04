#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace cluster::maps {
inline constexpr double earthRadius=6378137.0,pi=3.14159265358979323846;
inline constexpr double worldMetres=2*pi*earthRadius,mercatorLatitude=85.05112878;
struct WorldPoint {double east=0,north=0;};
inline double wrapEast(double x){return x-worldMetres*std::floor((x+worldMetres/2)/worldMetres);}
inline WorldPoint mercator(std::int32_t latE7,std::int32_t lonE7){
    const double lat=std::clamp(latE7/1e7,-mercatorLatitude,mercatorLatitude)*pi/180;
    return {wrapEast(lonE7/1e7*pi/180*earthRadius),earthRadius*std::log(std::tan(pi/4+lat/2))};
}
inline WorldPoint tilePoint(unsigned z,std::uint32_t x,std::uint32_t y,unsigned u,unsigned v){
    const double size=worldMetres/double(std::uint32_t(1)<<z);
    return {-worldMetres/2+(double(x)+u/4096.0)*size,worldMetres/2-(double(y)+v/4096.0)*size};
}
inline std::int32_t latitudeE7(double north){return std::clamp<std::int32_t>(std::int32_t(std::lround((2*std::atan(std::exp(std::clamp(north,-worldMetres/2,worldMetres/2)/earthRadius))-pi/2)*180/pi*1e7)),-850511287,850511287);}
inline std::int32_t longitudeE7(double east){return std::int32_t(std::lround(wrapEast(east)/earthRadius*180/pi*1e7));}
}
