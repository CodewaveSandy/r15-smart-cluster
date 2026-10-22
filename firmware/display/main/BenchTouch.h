#pragma once
#include "embedded/application/Lifecycle.h"
namespace cluster::board {
// Desk-test navigation only. ReadyForPowerOff remains powered over USB.
inline char benchTouchAction(Phase phase,int x,int y) {
    if(x<0||x>=800||y<0||y>=480) return 0;
    if(phase==Phase::ReadyForPowerOff) return 'b';
    if(phase!=Phase::Running) return 0;
    if(y<85) return x<400?'l':'t';
    if(y<376) return 'd';
    return x<300?'n':x<580?'i':'o';
}
}
