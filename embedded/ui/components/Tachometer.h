#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"
#include <array>

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class Tachometer {
public:
    explicit Tachometer(lv_obj_t* parent);
    Tachometer(const Tachometer&)=delete;
    Tachometer& operator=(const Tachometer&)=delete;
    void update(const Signal<int>& rpm,const Palette&);
private:
    lv_grad_dsc_t rampGradient_{};
    ObjectOwner rpmBed_, rpmValue_, caption_;
    lv_obj_t *rpmClip_, *rpmRamp_;
    std::array<ObjectOwner,14> ticks_{};
};
} // namespace cluster::ui
