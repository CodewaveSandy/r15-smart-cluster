#pragma once
#include "embedded/application/Clock.h"
#include <cstdio>
#include <cstring>

namespace cluster::presentation {
struct ClockText {char time[16]="—:—",date[32]="CLOCK NOT SYNCED",period[3]="";};
inline ClockText clockText(std::uint64_t epoch,int sourceOffset,const ClockPreferences& prefs) {
    ClockText out;
    if(epoch<Clock::firstEpoch||epoch>Clock::lastEpoch||!validClockPreferences(prefs))return out;
    const auto local=std::int64_t(epoch)+60*(prefs.automaticTimezone?sourceOffset:prefs.utcOffsetMinutes);
    const auto days=local/86400;const int seconds=int(local%86400);
    const int hour=seconds/3600,minute=seconds/60%60;
    // Gregorian civil date from days since Unix epoch (bounded to our clock range).
    const auto z=days+719468;const auto era=z/146097;
    const auto doe=z-era*146097;const auto yoe=(doe-doe/1460+doe/36524-doe/146096)/365;
    int year=int(yoe+era*400);const auto doy=doe-(365*yoe+yoe/4-yoe/100);
    const auto mp=(5*doy+2)/153;const int day=int(doy-(153*mp+2)/5+1);
    const int month=int(mp+(mp<10?3:-9));year+=month<=2;
    std::snprintf(out.time,sizeof out.time,"%02d:%02d",prefs.twelveHour?(hour%12?hour%12:12):hour,minute);
    if(prefs.twelveHour)std::strcpy(out.period,hour<12?"AM":"PM");
    if(prefs.dateFormat==DateFormat::YearMonthDay)std::snprintf(out.date,sizeof out.date,"%04d-%02d-%02d",year,month,day);
    else if(prefs.dateFormat==DateFormat::MonthDayYear)std::snprintf(out.date,sizeof out.date,"%02d/%02d/%04d",month,day,year);
    else std::snprintf(out.date,sizeof out.date,"%02d/%02d/%04d",day,month,year);
    return out;
}
}
