import QtQuick
import "../components"

Item {
    id: root
    required property QtObject shell
    required property QtObject tokens
    readonly property QtObject bike: shell.bike
    readonly property QtObject display: shell.display
    readonly property QtObject ride: shell.ride
    component Label: ClusterText { tokens: root.tokens }
    component Rule: Rectangle { height: 1; color: root.tokens.line }

    x: 28; y: 110
    Label { text: "VEHICLE / AT A GLANCE"; font.pixelSize: 20; font.bold: true; font.letterSpacing: 1 }
    Column {
        y: 46; spacing: 18
        Repeater {
            model: [
                {label: "MODEL", value: "Yamaha R15 V4 · 2026"},
                {label: "DATA SOURCE", value: "Desktop simulation · " + bike.dataStatus + " · no vehicle connected"},
                {label: "ODOMETER", value: shell.distance(ride.odometerKm, 1) + (display.imperial ? " mi" : " km") + " · demo reading"},
                {label: "BIKE BATTERY", value: bike.batteryVoltageAvailable ? bike.batteryVoltageV.toFixed(1) + " V · sample" : "Unavailable"},
                {label: "COOLANT", value: bike.coolantTemperatureAvailable ? Math.round(bike.coolantTemperatureC) + "°C · sample" : "Unavailable"},
                {label: "PHONE", value: !bike.available ? "Unavailable" : bike.phoneConnected ? "App connected · " + bike.phoneBatteryPercent + "% phone battery · simulated" : "Disconnected"},
                {label: "QS / VVA", value: !bike.available ? "Unavailable" : (bike.quickShifter ? "QS enabled" : "QS disabled") + " / " + (bike.vva ? "VVA active" : "VVA inactive")},
                {label: "VIN / ECU", value: "Not available until vehicle integration"}
            ]
            Item {
                required property var modelData
                width: 744; height: 17
                Label { text: parent.modelData.label; color: tokens.muted; font.pixelSize: 12 }
                Label { x: 170; text: parent.modelData.value; font.pixelSize: 14 }
            }
        }
    }

}
