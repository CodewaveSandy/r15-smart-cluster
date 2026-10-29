#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include <array>
#include <memory>
namespace cluster::ui {
class AppearancePanel {
public:
    explicit AppearancePanel(lv_obj_t* parent);
    ~AppearancePanel();
    void update(const ViewOptions&,const Palette&,int progress);
private:
    struct Preview;
    ObjectOwner root_;
    std::unique_ptr<Preview> preview_;
    lv_obj_t *title_,*themeTitle_,*back_,*design_,*designTitle_,*designStatus_,*status_;
    std::array<lv_obj_t*,2> themes_{},themeNames_{},themeStatus_{},buttons_{},buttonText_{};
};
}
