#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include <array>
namespace cluster::ui {
// First settings slice: four-category focus navigation; submenus follow separately.
class SettingsHome {
public:
    explicit SettingsHome(lv_obj_t* parent);
    void update(const ViewOptions&,const Palette&,int progress);
private:
    ObjectOwner root_;
    lv_obj_t *title_,*subtitle_,*back_;
    std::array<lv_obj_t*,4> tiles_{},icons_{},names_{},details_{};
};
}
