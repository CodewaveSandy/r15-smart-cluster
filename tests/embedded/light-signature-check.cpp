#include "embedded/presentation/LightSignatureAnimation.h"
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace cluster; using namespace cluster::presentation;
    int failures=0;
    const auto check=[&](bool ok,const char* why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    Lifecycle boot; boot.key(true,0);
    const auto b=[&](unsigned t){return animationFrame(boot,false,t);};
    check(b(0).stage==AnimationStage::Ignite&&!b(0).logo&&!b(0).lights,"Boot begins black");
    check(b(250).lights>0&&!b(250).logo&&b(250).blade<400,"Slender lights ignite before wordmark");
    check(b(349).stage==AnimationStage::Ignite&&b(350).stage==AnimationStage::Form,"Formation boundary");
    check(b(850).stage==AnimationStage::Hold&&b(1000).logo==255&&b(1000).lights==255,"Full signature");
    check(b(900).highlight>0,"Controlled highlight sweep");
    check(b(1200).stage==AnimationStage::Release&&b(1650).separation>0,"Blades move outward for release");
    check(b(1699).stage==AnimationStage::Release&&b(1700).stage==AnimationStage::Reveal,"Reveal boundary");
    check(b(1850).dashboardVisible&&b(1850).cover>0&&b(1850).cover<255,"Reveal blends current dashboard");
    boot.advance(1999); check(boot.phase()==Phase::Boot,"Boot cannot finish early");
    boot.advance(2000); check(boot.phase()==Phase::Running&&!animationFrame(boot,false,2000).overlay,"Exactly two-second boot");
    Lifecycle shutdown; shutdown.key(true,0); shutdown.advance(2000); shutdown.key(false,2000);
    const auto s=[&](unsigned t){return animationFrame(shutdown,false,2000+t);};
    check(s(0).cover==0&&s(0).dashboardVisible,"Shutdown begins from dashboard");
    check(s(100).cover>0&&s(100).cover<255,"Dashboard fade");
    check(s(200).stage==AnimationStage::Signature&&!s(200).dashboardVisible,"Signature after fade");
    check(s(400).stage==AnimationStage::Retract&&s(1000).trim>0,"Lights extinguish outside inward");
    check(s(1250).stage==AnimationStage::Farewell,"Farewell boundary");
    check(s(1750).logo>0&&s(1750).lights==0,"Small wordmark outlasts lights");
    check(s(1900).stage==AnimationStage::Black&&!s(1900).logo&&!s(1900).lights,"Final black interval");
    Lifecycle warningBoot; warningBoot.key(true,0);
    for(unsigned t:{0u,199u,625u,1100u,1875u}) {
        const auto urgent=animationFrame(shutdown,true,2000+t);
        check(!urgent.overlay&&urgent.dashboardVisible,"Urgent warning bypasses artwork immediately");
        const auto bootWarning=animationFrame(warningBoot,true,t);
        check(!bootWarning.overlay&&bootWarning.dashboardVisible,"Boot warning is immediate");
    }
    check(!animationFrame(warningBoot,true,1999).overlay,"Urgent warning bypasses final boot stage");
    // Every frame remains bounded and wordmark scale never grows beyond native.
    for(unsigned t=0;t<=2000;++t) for(const auto f:{b(t),s(t)}) {
        check(f.logoScale<=256&&f.logoScale>=180&&f.trim<=1000&&f.blade<=1000,"Bounded frame geometry");
    }
    shutdown.key(true,4800); check(shutdown.phase()==Phase::Running,"Key-on cancels shutdown");
    return failures?1:0;
}
