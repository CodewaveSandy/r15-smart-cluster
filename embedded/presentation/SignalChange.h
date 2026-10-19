#pragma once
#include "shared/model/ClusterState.h"
namespace cluster::presentation {
// Tracks only the last input, not a second text buffer or a telemetry backlog.
// Hidden values are immaterial until available; quality transitions still count.
template<class T> class SignalChange {
public:
    bool update(const Signal<T>& next) {
        const bool changed=!initialized_ || next.quality!=previous_.quality ||
            (next.available() && next.value!=previous_.value);
        previous_=next;
        initialized_=true;
        return changed;
    }
private:
    Signal<T> previous_{};
    bool initialized_=false;
};
} // namespace cluster::presentation
