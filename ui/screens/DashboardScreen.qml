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

    anchors.fill: parent
    TellTale { objectName: "leftSignal"; x: 28; y: 98; width: 40; kind: "left"; description: "Left indicator"; active: bike.leftIndicator && shell.blink; activeColor: tokens.accent; inactiveColor: tokens.muted }
    TellTale { x: 90; y: 98; width: 52; kind: "neutral"; description: "Neutral"; active: bike.available && bike.gear === 0; activeColor: tokens.accent; inactiveColor: tokens.muted }
    TellTale { x: 161; y: 98; width: 52; kind: "beam"; description: "High beam"; active: bike.highBeam; activeColor: tokens.beam; inactiveColor: tokens.muted }
    TellTale { x: 232; y: 98; width: 52; kind: "tcs"; description: "Traction control"; active: bike.tcsWarning; activeColor: tokens.amber; inactiveColor: tokens.muted }
    TellTale { x: 303; y: 98; width: 52; kind: "abs"; description: "ABS warning"; active: bike.absWarning; activeColor: tokens.amber; inactiveColor: tokens.muted }
    TellTale { x: 374; y: 98; width: 52; kind: "engine"; description: "Engine warning"; active: bike.engineWarning; activeColor: tokens.amber; inactiveColor: tokens.muted }
    TellTale { x: 445; y: 98; width: 52; kind: "temp"; description: "Coolant warning"; active: bike.coolantWarning; activeColor: tokens.red; inactiveColor: tokens.muted }
    TellTale { x: 516; y: 98; width: 52; kind: "message"; description: "Message"; active: bike.available && bike.messagePending; activeColor: tokens.accent; inactiveColor: tokens.muted }
    TellTale { x: 587; y: 98; width: 52; kind: "call"; description: "Incoming call"; active: bike.available && bike.incomingCall; activeColor: tokens.accent; inactiveColor: tokens.muted }
    TellTale { x: 658; y: 98; width: 52; kind: "hazard"; description: "Hazards"; active: bike.hazards && shell.blink; activeColor: tokens.red; inactiveColor: tokens.muted }
    TellTale { objectName: "rightSignal"; x: 732; y: 98; width: 40; kind: "right"; description: "Right indicator"; active: bike.rightIndicator && shell.blink; activeColor: tokens.accent; inactiveColor: tokens.muted }
    // The larger track tachometer retains the same color thresholds as Street.
    Tachometer { x: 28; y: 144; width: 744; rpm: bike.engineRpm; track: display.track; available: bike.available; tokens: root.tokens }
    Label {
        objectName: "speedValue"
        x: display.track ? 28 : 210; y: display.track ? 231 : 220
        width: display.track ? 255 : 380; horizontalAlignment: display.track ? Text.AlignLeft : Text.AlignHCenter
        text: !bike.available ? "—" : Math.round(bike.speedKph * (display.imperial ? 0.621371 : 1)); font.pixelSize: display.track ? 79 : 128; font.weight: Font.Bold
    }
    Label { x: display.track ? 225 : 590; y: display.track ? 277 : 306; text: display.imperial ? "mph" : "km/h"; color: tokens.muted; font.pixelSize: 16 }
    Item {
        visible: display.track
        x: 365; y: 239
        Label { text: "LAP " + ride.lapNumber.toString().padStart(2, "0") + (ride.lapRunning ? "  /  RUNNING" : ride.currentLapMs > 0 ? "  /  STOPPED" : "  /  READY"); color: tokens.accent; font.pixelSize: 10; font.letterSpacing: 2 }
        Label { y: 20; text: shell.lap(ride.currentLapMs); font.pixelSize: 42; font.weight: Font.DemiBold }
        Label { y: 72; text: "K  start / next lap     B  stop"; color: tokens.muted; font.pixelSize: 10 }
    }
    Rule { x: 28; y: display.track ? 336 : 376; width: 744 }
    Label { x: 28; y: display.track ? 351 : 386; text: shell.activePage; color: tokens.accent; font.pixelSize: 10; font.letterSpacing: 2 }
    Label { x: 28; y: display.track ? 367 : 399; text: shell.pageValue(shell.activePage); font.pixelSize: display.track ? 31 : 26; font.weight: Font.DemiBold }
    Label { x: display.track ? 29 : 203; y: display.track ? 408 : 411; text: shell.pageUnit(shell.activePage); color: tokens.muted; font.pixelSize: 10 }
    Label { x: 267; y: display.track ? 351 : 386; text: display.track ? "FASTEST LAP" : "BIKE / COOLANT"; color: tokens.muted; font.pixelSize: 10; font.letterSpacing: 1 }
    Label { x: 267; y: display.track ? 377 : 402; text: !bike.available ? "—" : display.track ? shell.lap(ride.fastestLapMs) : (bike.batteryVoltageAvailable ? bike.batteryVoltageV.toFixed(1) : "—") + " V  /  " + (bike.coolantTemperatureAvailable ? Math.round(bike.coolantTemperatureC) : "—") + "°C"; font.pixelSize: 23 }
    Label { visible: display.track; x: 267; y: 409; text: display.track ? "LATEST  " + shell.lap(ride.latestLapMs) : "Enter  cycle information"; color: tokens.muted; font.pixelSize: 10 }
    Rectangle { x: 537; y: display.track ? 353 : 387; width: 1; height: display.track ? 65 : 40; color: tokens.line }
    FuelGauge {
        objectName: "fuelGauge"
        x: 552; y: display.track ? 348 : 383; width: 220; height: display.track ? 80 : 55
        compact: !display.track
        fuelPercent: bike.fuelPercent; available: bike.available
        foreground: tokens.ink; muted: tokens.muted; depleted: tokens.panel
        accent: tokens.accent; warning: tokens.amber; critical: tokens.red
        fontFamily: Qt.application.font.family
    }

}
