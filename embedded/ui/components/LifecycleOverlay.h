#pragma once
#include "../ObjectOwner.h"
#include "embedded/presentation/LightSignatureAnimation.h"

namespace cluster::ui {
// Parent outlives this owner. Callback and images live on the LVGL owner thread.
class LifecycleOverlay {
public:
    explicit LifecycleOverlay(lv_obj_t* parent);
    LifecycleOverlay(const LifecycleOverlay&)=delete;
    LifecycleOverlay& operator=(const LifecycleOverlay&)=delete;
    void update(const presentation::AnimationFrame&);
private:
    static void draw(lv_event_t*);
    ObjectOwner overlay_;
    lv_obj_t* wordmark_;
    presentation::AnimationFrame frame_{};
    bool initialized_=false;
};
} // namespace cluster::ui
