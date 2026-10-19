#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"
#include "embedded/application/Lifecycle.h"

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class LifecycleOverlay {
public:
    explicit LifecycleOverlay(lv_obj_t* parent);
    LifecycleOverlay(const LifecycleOverlay&)=delete;
    LifecycleOverlay& operator=(const LifecycleOverlay&)=delete;
    void update(const Lifecycle&,bool urgentWarning,const Palette&,std::uint64_t now);
private:
    ObjectOwner overlay_;
    lv_obj_t *animationTitle_, *animationSubtitle_, *animationBed_, *animationFill_;
};
} // namespace cluster::ui
