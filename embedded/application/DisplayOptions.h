#pragma once
#include "DashboardPages.h"
namespace cluster {
// Display-local preferences and navigation. No vehicle commands or persistence.
struct DisplayOptions {
    bool light=false, track=false, imperial=false;
    bool settings=false;
    unsigned settingsFocus=0;
    bool appearance=false, appearanceOriginalLight=false, appearanceSaveFailed=false, appearanceSaving=false;
    unsigned appearanceFocus=0;
    bool phone=false,phoneForgetConfirmation=false;
    unsigned phoneFocus=0;
    bool bluetooth=false, bluetoothForgetConfirmation=false;
    unsigned bluetoothFocus=0;
    bool wifi=false, wifiForgetConfirmation=false, wifiCredentialNotice=false;
    unsigned wifiFocus=0, wifiNetwork=0;
    TripPage tripPage=TripPage::Odometer;
    InfoPage infoPage=InfoPage::Coolant;
};
} // namespace cluster
