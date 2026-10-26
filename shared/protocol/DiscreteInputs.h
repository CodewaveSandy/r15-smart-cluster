#pragma once
#include "shared/model/ClusterState.h"
#include <array>
namespace cluster::inputs {
// One measured discrete signal for each position; never infer N from no selection.
inline Signal<int> gear(const std::array<bool,7>& asserted) {
    Signal<int> result;
    for(int i=0;i<7;++i)if(asserted[i]) {
        if(result.available())return {0,Quality::Invalid};
        result={i,Quality::Available};
    }
    return result;
}
}
