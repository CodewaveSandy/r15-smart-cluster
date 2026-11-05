#pragma once
#include "shared/model/TurnGuidanceState.h"
#include <cstdio>
#include <cstring>
namespace cluster::presentation {
struct TurnCard {
    bool visible=false,emphasis=false;guidance::Maneuver maneuver=guidance::Maneuver::Unknown;unsigned exit=0;
    char instruction[100]{},road[64]{},summary[100]{},distance[20]{};
};
inline void guidanceDistance(char* b,unsigned n,std::uint32_t metres){
    if(metres==guidance::unknown)std::snprintf(b,n,"-- m");
    else if(metres<1000)std::snprintf(b,n,"%u m",unsigned(metres));
    else std::snprintf(b,n,metres<10000?"%.1f km":"%.0f km",metres/1000.0);
}
inline TurnCard turnCard(const guidance::TurnGuidanceState& s){
    using namespace guidance;TurnCard p;p.visible=s.visible;if(!p.visible)return p;
    const auto say=[&](const char* text){std::snprintf(p.instruction,sizeof p.instruction,"%s",text);};
    if(s.quality==guidance::Quality::LinkLost){say("Guidance connection lost");return p;}
    if(s.quality==guidance::Quality::Cleared){
        say(s.reason==Reason::PermissionRevoked?"Notification access revoked":s.reason==Reason::ListenerLost?"Notification listener unavailable":s.reason==Reason::Unsupported?"Maps notification unsupported":"Google Maps guidance ended");return p;
    }
    if(s.quality==guidance::Quality::Pending){say("Guidance update pending");std::snprintf(p.road,sizeof p.road,"Check Google Maps on your phone");return p;}
    if(s.quality==guidance::Quality::Unsupported){say("Maps notification unsupported");return p;}
    if(s.content.source!=Source::Guiding&&s.content.source!=Source::Arrived){
        say(s.content.source==Source::Rerouting?"Rerouting...":s.content.source==Source::GpsLost?"Google Maps GPS unavailable":s.content.source==Source::Paused?"Google Maps guidance paused":"Waiting for Maps guidance");return p;
    }
    say(s.content.instruction[0]?s.content.instruction.data():"Arrived");std::snprintf(p.road,sizeof p.road,"%s",s.content.road.data());
    const bool fresh=s.quality==guidance::Quality::Fresh&&!(s.content.flags&Truncated);
    p.emphasis=fresh;p.maneuver=s.action?s.content.maneuver:Maneuver::Unknown;p.exit=s.action?s.content.exit:0;
    if(fresh&&s.content.source==Source::Guiding)guidanceDistance(p.distance,sizeof p.distance,s.content.flags&NextDistance?s.content.next:unknown);
    if(s.quality==guidance::Quality::LastUpdate)std::snprintf(p.summary,sizeof p.summary,"Last Maps update %u s ago",unsigned(s.ageMs/1000));
    else if(s.quality==guidance::Quality::UnknownAge)std::snprintf(p.summary,sizeof p.summary,"Age unknown - check Google Maps");
    else if(s.content.flags&Truncated)std::snprintf(p.summary,sizeof p.summary,"Shortened notification - check Google Maps");
    else if(fresh){
        char remaining[24]{};if(s.content.flags&RemainingDistance)guidanceDistance(remaining,sizeof remaining,s.content.remaining);
        if(s.content.flags&RemainingTime)std::snprintf(p.summary,sizeof p.summary,"Reported: %s%s%u min%s%s",remaining,remaining[0]?" / ":"",unsigned((s.content.seconds+59)/60),s.content.summary[0]?" / ":"",s.content.summary.data());
        else std::snprintf(p.summary,sizeof p.summary,"%s%s%s",remaining,remaining[0]&&s.content.summary[0]?" / ":"",s.content.summary.data());
    }
    return p;
}
}
