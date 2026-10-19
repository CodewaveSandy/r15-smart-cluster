#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class RideInfoPanel {
public:
    explicit RideInfoPanel(lv_obj_t* parent);
    RideInfoPanel(const RideInfoPanel&)=delete;
    RideInfoPanel& operator=(const RideInfoPanel&)=delete;
    void update(const ClusterState&,const ViewOptions&,const Palette&);
private:
    ObjectOwner lowerRule_, tripTitle_, tripValue_, tripUnit_, infoTitle_, infoValue_;
};
} // namespace cluster::ui
