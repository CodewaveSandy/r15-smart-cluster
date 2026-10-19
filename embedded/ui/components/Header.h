#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "embedded/presentation/SignalChange.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"
#include <array>

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class Header {
public:
    explicit Header(lv_obj_t* parent);
    Header(const Header&)=delete;
    Header& operator=(const Header&)=delete;
    void update(const ClusterState&,const ViewOptions&,const Palette&);
private:
    presentation::SignalChange<int> lastGear_,lastPhoneBattery_;
    presentation::SignalChange<bool> lastPhoneConnection_;
    ObjectOwner header_;
    lv_obj_t *gear_, *mode_, *subtitle_, *clock_, *date_, *phone_, *qs_, *vva_, *phoneBattery_;
    std::array<ObjectOwner,2> rules_{};
};
} // namespace cluster::ui
