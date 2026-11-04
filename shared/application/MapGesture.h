#pragma once
#include "MapCamera.h"
#include <cstdlib>
namespace cluster::maps {
enum class TouchPhase { Press, Move, Release, Cancel };
struct Touch {TouchPhase phase=TouchPhase::Cancel;int x=0,y=0;};
// One-contact reducer. A second contact, I2C fault, hidden page or lifecycle
// change cancels; a drag must never become a footer/bench action on release.
class MapGesture {
public:
    void cancel(){tracking_=dragging_=false;}
    bool consume(Touch t,MapCamera& camera){
        if(t.phase==TouchPhase::Cancel){const bool was=tracking_;cancel();return was;}
        if(t.phase==TouchPhase::Press){
            cancel();if(t.x<0||t.x>=800||t.y<142||t.y>=442)return false;
            tracking_=true;startX_=lastX_=t.x;startY_=lastY_=t.y;return true;
        }
        if(!tracking_)return false;
        if(t.phase==TouchPhase::Move){
            if(!dragging_&&(std::abs(t.x-startX_)>=6||std::abs(t.y-startY_)>=6))dragging_=true;
            if(dragging_){camera.drag(t.x-lastX_,t.y-lastY_);lastX_=t.x;lastY_=t.y;}
        }
        if(t.phase==TouchPhase::Release)cancel();
        return true;
    }
private:bool tracking_=false,dragging_=false;int startX_=0,startY_=0,lastX_=0,lastY_=0;
};
}
