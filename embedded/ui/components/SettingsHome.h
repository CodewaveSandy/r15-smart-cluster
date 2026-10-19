#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include <array>
namespace cluster::ui {
// Category navigation; Appearance and radio panels share the instrument strip.
class SettingsHome {
public:
    explicit SettingsHome(lv_obj_t* parent);
    void update(const ViewOptions&,const BluetoothState&,const WifiState&,const Palette&,int progress);
private:
    ObjectOwner root_;
    lv_obj_t *title_,*subtitle_,*back_;
    std::array<lv_obj_t*,4> tiles_{},icons_{},names_{},details_{};
};
}
