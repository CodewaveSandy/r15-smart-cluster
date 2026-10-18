#include "Icons.h"
#include <cassert>
namespace cluster::ui {
lv_obj_t* createIcon(lv_obj_t* parent,int x,int y,Icon kind) {
    const char* glyph=nullptr;
    for(const auto& definition:iconDefinitions) if(definition.kind==kind) glyph=definition.glyph;
    assert(glyph); // Internal enum only; no transport input is accepted here.
    auto* object=lv_label_create(parent);
    lv_obj_set_pos(object,x,y); lv_obj_set_size(object,32,32);
    lv_obj_set_style_text_font(object,iconFont(kind),0);
    lv_obj_set_style_text_align(object,LV_TEXT_ALIGN_CENTER,0);
    lv_label_set_long_mode(object,LV_LABEL_LONG_CLIP);
    lv_label_set_text_static(object,glyph?glyph:"");
    return object;
}
}
