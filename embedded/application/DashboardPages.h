#pragma once
#include "shared/model/ClusterState.h"
namespace cluster {
enum class TripPage { Odometer, Trip1, Trip2, Count };
enum class InfoPage { Coolant, Track, RealtimeFuel, AverageFuel, AverageSpeed, Tcs, Unidentified, Count };
constexpr TripPage next(TripPage page) { return static_cast<TripPage>((static_cast<unsigned>(page)+1)%static_cast<unsigned>(TripPage::Count)); }
constexpr InfoPage next(InfoPage page) { return static_cast<InfoPage>((static_cast<unsigned>(page)+1)%static_cast<unsigned>(InfoPage::Count)); }
inline const char* tripTitle(TripPage page) {
    switch(page) {
    case TripPage::Odometer:return "ODO";
    case TripPage::Trip1:return "TRIP 1";
    case TripPage::Trip2:return "TRIP 2";
    case TripPage::Count:break;
    }
    return "—";
}
inline Signal<double> tripReading(const ClusterState& state,TripPage page) {
    switch(page) {
    case TripPage::Odometer:return state.odometerKm;
    case TripPage::Trip1:return state.trip1Km;
    case TripPage::Trip2:return state.trip2Km;
    case TripPage::Count:break;
    }
    return {};
}
}
