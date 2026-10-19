#pragma once
#include "lvgl.h"
#include <memory>

namespace cluster::ui {
struct DeleteObject {
    void operator()(lv_obj_t* object) const noexcept { lv_obj_delete(object); }
};
// Own only top-level objects. LVGL recursively destroys their descendants.
// The containing parent must outlive this owner. Destruction is UI-thread-only.
using ObjectOwner=std::unique_ptr<lv_obj_t,DeleteObject>;
} // namespace cluster::ui
