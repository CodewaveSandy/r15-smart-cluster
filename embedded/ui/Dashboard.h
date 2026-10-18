#pragma once
#include "lvgl.h"
#include "shared/model/ClusterState.h"
#include "embedded/application/Lifecycle.h"
#include "Tokens.h"
#include <array>
#include <cstdint>
namespace cluster::ui {
// Owns one LVGL subtree. Construct/update/destroy only on the LVGL owner thread.
class Dashboard {
public:
    explicit Dashboard(lv_obj_t* parent);
    ~Dashboard();
    Dashboard(const Dashboard&)=delete;
    Dashboard& operator=(const Dashboard&)=delete;
    void update(const ClusterState&, const ViewOptions&, const Lifecycle&, std::uint64_t now);
private:
    lv_obj_t *root_, *header_, *gear_, *mode_, *subtitle_, *clock_, *date_, *phone_, *qs_, *vva_, *phoneBattery_;
    lv_obj_t *rpmBed_, *rpmClip_, *rpmRamp_, *rpmValue_, *speed_, *unit_;
    lv_obj_t *lowerRule_, *tripTitle_, *tripValue_, *tripUnit_, *infoTitle_, *infoValue_, *lap_, *lapHint_;
    lv_obj_t *fuelTitle_, *fuelValue_, *pump_, *fuelDivider_;
    lv_obj_t *overlay_, *animationTitle_, *animationSubtitle_, *animationBed_, *animationFill_;
    std::array<lv_obj_t*,8> fuel_{};
    std::array<lv_obj_t*,11> lamps_{};
    std::array<lv_obj_t*,14> ticks_{};
    std::array<lv_obj_t*,2> rules_{};
    lv_grad_dsc_t rampGradient_{};
    lv_grad_dsc_t shiftGradient_{};
};
}
