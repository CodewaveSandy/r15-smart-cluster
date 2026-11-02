#pragma once
#include "shared/model/MapScene.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>

namespace cluster::host {
// Host-only text-file adapter. No file decoding or geographic data in widgets.
inline maps::Scene loadMapFile(const std::filesystem::path& path) {
    if(std::filesystem::file_size(path)>2*1024*1024)throw std::runtime_error("Map file exceeds 2 MiB");
    std::ifstream file(path);std::string magic,name;unsigned version=0,count=0;
    double latitude=0,longitude=0;
    if(!(file>>magic>>version)||magic!="R15MAP"||version!=1)throw std::runtime_error("Unsupported map file");
    file.ignore(256,'\n');std::getline(file,name);
    if(name!="Kolkata, West Bengal"||!(file>>latitude>>longitude>>count)||
        !std::isfinite(latitude)||!std::isfinite(longitude)||latitude<22||latitude>23||longitude<88||longitude>89||count==0||count>maps::Scene{}.roads.size())
        throw std::runtime_error("Invalid Kolkata map header/capacity");
    maps::Scene scene;scene.quality=Quality::Available;scene.roadCount=count;
    for(unsigned i=0;i<count;++i) {
        auto& road=scene.roads[i];unsigned kind=0;std::string label;
        if(!(file>>kind>>road.count>>std::quoted(label))||(kind!=0&&kind!=1&&kind!=3&&kind!=4)||road.count<2||road.count>road.points.size()||label.size()>=road.name.size())
            throw std::runtime_error("Invalid map road header");
        road.kind=static_cast<maps::RoadKind>(kind);std::copy(label.begin(),label.end(),road.name.begin());
        for(unsigned j=0;j<road.count;++j)if(!(file>>road.points[j].east>>road.points[j].north))throw std::runtime_error("Truncated map coordinates");
    }
    file>>std::ws;if(file.peek()!=std::char_traits<char>::eof()||!maps::valid(scene))throw std::runtime_error("Malformed map scene");
    return scene;
}
} // namespace cluster::host
