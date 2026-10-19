#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class PrimaryReadout {
public:
    explicit PrimaryReadout(lv_obj_t* parent);
    PrimaryReadout(const PrimaryReadout&)=delete;
    PrimaryReadout& operator=(const PrimaryReadout&)=delete;
    void update(const ClusterState&,const ViewOptions&,const Palette&);
private:
    ObjectOwner speed_, unit_, lap_, lapHint_, speedTitle_;
    bool initialized_=false, previousTrack_=false;
};
} // namespace cluster::ui
