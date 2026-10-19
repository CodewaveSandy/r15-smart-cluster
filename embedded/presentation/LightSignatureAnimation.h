#pragma once
#include "embedded/application/Lifecycle.h"
#include <algorithm>
#include <cstdint>

namespace cluster::presentation {
enum class AnimationStage { Running, Black, Ignite, Form, Hold, Release, Reveal, FadeOut, Signature, Retract, Farewell };
struct AnimationFrame {
    AnimationStage stage=AnimationStage::Black;
    bool overlay=true,dashboardVisible=false;
    std::uint8_t cover=255,logo=0,lights=0,highlight=0;
    std::uint16_t blade=200,trim=0,logoScale=256,highlightPosition=0;
    std::int16_t separation=0;
};
inline float interval(std::uint64_t t,unsigned start,unsigned end) {
    return std::clamp((float(t)-float(start))/float(end-start),0.0f,1.0f);
}
inline float smooth(float x) { return x*x*(3.0f-2.0f*x); }
inline std::uint8_t opacity(float x) { return static_cast<std::uint8_t>(std::clamp(x,0.0f,255.0f)); }
inline AnimationFrame animationFrame(const Lifecycle& life,bool urgentWarning,std::uint64_t now) {
    AnimationFrame f;
    const auto phase=life.phase();
    // Keep the approved phase proportions with independent lifecycle durations.
    const auto duration=phase==Phase::Boot?Lifecycle::bootMs:Lifecycle::shutdownMs;
    const auto t=std::min(life.elapsed(now),duration)*2000/duration;
    if(phase==Phase::Running || ((phase==Phase::Boot||phase==Phase::Shutdown)&&urgentWarning)) {
        f.stage=AnimationStage::Running; f.overlay=false; f.dashboardVisible=true; f.cover=0;
        return f;
    }
    if(phase==Phase::Boot) {
        if(t<350) {
            f.stage=AnimationStage::Ignite;
            f.lights=opacity(230*smooth(interval(t,0,350)));
            f.blade=std::uint16_t(150+200*interval(t,0,350));
        } else if(t<850) {
            f.stage=AnimationStage::Form;
            const float p=smooth(interval(t,350,850));
            f.lights=opacity(230+25*p); f.blade=std::uint16_t(350+650*p);
            f.logo=opacity(255*smooth(interval(t,400,750)));
        } else if(t<1200) {
            f.stage=AnimationStage::Hold; f.lights=255; f.blade=1000; f.logo=255;
        } else if(t<1700) {
            f.stage=AnimationStage::Release;
            const float p=smooth(interval(t,1200,1700));
            f.lights=opacity(255-191*p); f.blade=std::uint16_t(1000-800*p);
            f.logo=opacity(255*(1-p)); f.separation=std::int16_t(30*p);
        } else {
            f.stage=AnimationStage::Reveal;
            const float p=smooth(interval(t,1700,2000));
            f.cover=opacity(255*(1-p)); f.dashboardVisible=true;
            f.lights=opacity(64*(1-p)); f.blade=200; f.separation=30;
        }
        if(t>=700&&t<1100) {
            const float p=interval(t,700,1100);
            f.highlight=opacity(220*(1-(2*p-1)*(2*p-1)));
            f.highlightPosition=std::uint16_t(1000*p);
        }
    } else if(phase==Phase::Shutdown) {
        if(t<200) {
            f.stage=AnimationStage::FadeOut; f.dashboardVisible=true;
            f.cover=opacity(255*smooth(interval(t,0,200)));
        } else if(t<400) {
            f.stage=AnimationStage::Signature;
            const float p=smooth(interval(t,200,400));
            f.lights=opacity(255*p); f.logo=opacity(255*p); f.blade=1000;
        } else if(t<1250) {
            f.stage=AnimationStage::Retract;
            const float p=smooth(interval(t,400,1250));
            f.lights=opacity(255-75*p); f.logo=255;
            f.trim=std::uint16_t(800*p); f.blade=std::uint16_t(1000-600*p);
            f.logoScale=std::uint16_t(256-48*p);
        } else if(t<1900) {
            f.stage=AnimationStage::Farewell;
            const float p=smooth(interval(t,1250,1900));
            f.lights=opacity(180*(1-smooth(interval(t,1250,1600))));
            f.trim=std::uint16_t(800+200*interval(t,1250,1600)); f.blade=400;
            f.logoScale=std::uint16_t(208-28*p); f.logo=opacity(255*(1-p));
        }
    }
    return f;
}
} // namespace cluster::presentation
