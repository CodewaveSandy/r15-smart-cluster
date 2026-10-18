#pragma once
#include "shared/model/ClusterState.h"
namespace cluster {
enum class TripPage { Odometer, TripA, TripB, TripF, Count };
enum class InfoPage { Coolant, Battery, Tcs, Track, Count };
constexpr TripPage next(TripPage page) { return static_cast<TripPage>((static_cast<unsigned>(page)+1)%static_cast<unsigned>(TripPage::Count)); }
constexpr InfoPage next(InfoPage page) { return static_cast<InfoPage>((static_cast<unsigned>(page)+1)%static_cast<unsigned>(InfoPage::Count)); }
inline const char* tripTitle(TripPage page) {
    switch(page) {
    case TripPage::Odometer:return "ODO";
    case TripPage::TripA:return "TRIP A";
    case TripPage::TripB:return "TRIP B";
    case TripPage::TripF:return "TRIP F";
    case TripPage::Count:break;
    }
    return "—";
}
inline Signal<double> tripReading(const ClusterState& state,TripPage page) {
    switch(page) {
    case TripPage::Odometer:return state.odometerKm;
    case TripPage::TripA:return state.tripAKm;
    case TripPage::TripB:return state.tripBKm;
    case TripPage::TripF:return state.tripFKm;
    case TripPage::Count:break;
    }
    return {};
}
}
