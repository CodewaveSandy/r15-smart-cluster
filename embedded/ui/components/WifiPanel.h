#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/WifiState.h"
#include <array>
namespace cluster::ui {
class WifiPanel {
public:
    explicit WifiPanel(lv_obj_t* parent);
    void update(const WifiState&,const ViewOptions&,const Palette&,int progress);
private:
    ObjectOwner root_;
    lv_obj_t *title_,*subtitle_,*back_,*card_,*icon_,*name_,*status_;
    std::array<lv_obj_t*,5> rows_{},labels_{},arrows_{};
};
}
