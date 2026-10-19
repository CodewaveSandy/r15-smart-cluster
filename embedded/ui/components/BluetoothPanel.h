#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/BluetoothState.h"
#include <array>
namespace cluster::ui {
class BluetoothPanel {
public:
    explicit BluetoothPanel(lv_obj_t* parent);
    void update(const BluetoothState&,const ViewOptions&,const Palette&,int progress);
private:
    ObjectOwner root_;
    lv_obj_t *title_,*subtitle_,*back_,*card_,*phone_,*name_,*status_;
    std::array<lv_obj_t*,3> rows_{},labels_{},arrows_{};
};
}
