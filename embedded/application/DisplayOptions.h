#pragma once
#include "DashboardPages.h"
namespace cluster {
// Display-local preferences and navigation. No vehicle commands or persistence.
struct DisplayOptions {
    bool light=false, track=false, imperial=false;
    TripPage tripPage=TripPage::Odometer;
    InfoPage infoPage=InfoPage::Coolant;
};
} // namespace cluster
