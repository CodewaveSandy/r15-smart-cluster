#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/MapScene.h"
#include <memory>

namespace cluster::ui {
struct MapStyle {
    std::uint32_t ground,road,roadEdge,water,route,routeEdge,label,marker;
};
inline constexpr MapStyle mapNight{0x101c29,0x34495b,0x172938,0x193a50,0x68baff,0x1c526e,0xa8bccd,0xffffff};
inline constexpr MapStyle mapDay{0xe7ede7,0xffffff,0xbac7c6,0xb8dcec,0x1667bf,0xffffff,0x3e565e,0xffffff};
struct MapGeometry { static constexpr int x=0,y=142,width=800,height=300; };

// Persistent owner-thread renderer. Firmware borrows an immutable validated
// scene; the host copy helper allocates lazily. Draws never load network/files.
class MapPanel {
public:
    explicit MapPanel(lv_obj_t* parent);
    bool setScene(const maps::Scene&); // host convenience, lazily owned heap copy
    void bindScene(const maps::Scene*); // caller keeps immutable storage alive
    void visible(bool);
    void update(const maps::Pose&,bool overview,bool light,unsigned zoom=1,bool browsing=false);
private:
    static void draw(lv_event_t*);
    std::unique_ptr<maps::Scene> owned_;
    const maps::Scene* scene_=nullptr;
    maps::Pose pose_{};
    bool initialized_=false,overview_=false,light_=false,browsing_=false;
    unsigned zoom_=1;
    ObjectOwner root_;
};
} // namespace cluster::ui
