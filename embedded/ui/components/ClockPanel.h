#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include <array>
namespace cluster::ui {
class ClockPanel {
public:
    explicit ClockPanel(lv_obj_t* parent);
    void update(const ViewOptions&,const Palette&,int progress);
private:
    ObjectOwner root_;
    lv_obj_t *title_,*back_,*status_;
    std::array<lv_obj_t*,7> rows_{},labels_{};
};
}
