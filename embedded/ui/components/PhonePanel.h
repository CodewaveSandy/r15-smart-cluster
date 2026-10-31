#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/PhoneState.h"
#include <array>
namespace cluster::ui {
class PhonePanel {
public:
    explicit PhonePanel(lv_obj_t* parent);
    void update(const PhoneState&,const ViewOptions&,const Palette&,int progress);
private:
    ObjectOwner root_;
    lv_obj_t *title_,*subtitle_,*back_,*card_,*phone_,*name_,*status_;
    std::array<lv_obj_t*,3> rows_{},labels_{},arrows_{};
    lv_obj_t *modal_,*modalCard_,*modalTitle_,*modalIdentity_,*modalCode_,*modalHint_;
    lv_obj_t *allow_,*reject_,*allowLabel_,*rejectLabel_;
};
}
