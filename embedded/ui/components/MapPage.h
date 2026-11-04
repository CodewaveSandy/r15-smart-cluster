#pragma once
#include "MapPanel.h"
#include "embedded/application/DisplayOptions.h"
namespace cluster::ui {
struct MapPageState {
    const maps::Scene* scene=nullptr;maps::Pose pose;
    const char* transfer="Connect phone to share a map";
    const char* location="Waiting for location";
    bool navigation=false;
    const char* coverage="";
    const char* guidance="";
    const char* attribution="OpenStreetMap contributors";
};
class MapPage {
public:
    explicit MapPage(lv_obj_t* parent);
    void update(const MapPageState&,const ViewOptions&,const Palette&,bool visible);
private:
    ObjectOwner root_;MapPanel panel_;
    lv_obj_t *card_,*licence_,*title_,*detail_,*coverage_,*routeCard_,*guidance_,*north_,*mode_,*scale_,*footer_,*copyright_,*attribution_;
    std::array<lv_obj_t*,6> controls_{};
};
}
