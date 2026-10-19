#pragma once
#include "lvgl.h"
#include <cstring>
#include <cstddef>

namespace cluster::ui {
// Component-owned storage must outlive its label and never move after binding.
// Presenter outputs are bounded arrays; reject oversized inputs at compile time.
class ReadoutText {
public:
    ReadoutText()=default;
    ReadoutText(const ReadoutText&)=delete;
    ReadoutText& operator=(const ReadoutText&)=delete;
    template<std::size_t N> void update(lv_obj_t* label,const char (&value)[N]) {
        static_assert(N<=sizeof value_,"Readout exceeds persistent text capacity");
        if(std::strcmp(lv_label_get_text(label),value)==0) return;
        std::memcpy(value_,value,N);
        lv_label_set_text_static(label,value_);
    }
private:
    char value_[32]{};
};
} // namespace cluster::ui
