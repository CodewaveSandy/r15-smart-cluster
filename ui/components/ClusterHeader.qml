import QtQuick

Item {
    id: root
    required property QtObject bike
    required property QtObject display
    required property QtObject tokens
    component Label: ClusterText { tokens: root.tokens }
    component Rule: Rectangle { height: 1; color: root.tokens.line }
    component Lamp: Rectangle {
        property string label
        property bool active: false
        property color activeColor: tokens.amber
        width: lampText.implicitWidth + 18; height: 25; radius: 5
        color: active ? activeColor : tokens.panel
        Label { id: lampText; anchors.centerIn: parent; text: parent.label; color: parent.active ? (tokens.lightPalette ? "#ffffff" : "#101820") : tokens.muted; font.pixelSize: 10; font.bold: true }
    }
    // Gear is always the first element, followed by mode and OEM status information.
    Label { x: 28; y: 19; text: "GEAR"; color: tokens.muted; font.pixelSize: 9; font.letterSpacing: 2 }
    Label { x: 28; y: 30; text: !bike.available ? "—" : bike.gear === 0 ? "N" : bike.gear; color: !bike.available ? tokens.muted : bike.gear === 0 ? tokens.accent : tokens.ink; font.pixelSize: 46; font.bold: true }
    Rectangle { x: 96; y: 22; width: 1; height: 48; color: tokens.line }
    Label { x: 120; y: 26; text: display.track ? "TRACK" : "STREET"; font.pixelSize: 22; font.bold: true; font.letterSpacing: 2 }
    Label { x: 121; y: 55; text: display.track ? "LAP / PERFORMANCE" : "ROAD / EVERYDAY"; font.pixelSize: 9; color: tokens.muted; font.letterSpacing: 1 }
    Lamp { objectName: "qsLamp"; x: 288; y: 34; label: "QS"; visible: bike.available && bike.quickShifter; active: true; activeColor: tokens.accent }
    Lamp { objectName: "vvaLamp"; x: 344; y: 34; label: "VVA"; visible: bike.available && bike.vva; active: true; activeColor: tokens.amber }
    TellTale { x: 448; y: 34; kind: "phone"; description: "Phone connection"; active: bike.available && bike.phoneConnected; activeColor: tokens.accent; inactiveColor: tokens.muted }
    Rectangle {
        x: 508; y: 37; width: 33; height: 17; radius: 2; color: "transparent"; border.color: bike.available && bike.phoneConnected ? tokens.accent : tokens.muted
        Rectangle { x: 34; y: 5; width: 3; height: 7; color: tokens.muted }
        Rectangle { x: 3; y: 3; height: 11; width: bike.available && bike.phoneConnected ? 27 * bike.phoneBatteryPercent / 100 : 0; color: tokens.accent }
    }
    Label { x: 551; y: 37; text: bike.available && bike.phoneConnected ? bike.phoneBatteryPercent + "%" : "—"; color: tokens.muted; font.pixelSize: 13 }
    Label { x: 658; y: 23; width: 114; horizontalAlignment: Text.AlignRight; text: Qt.formatDateTime(display.now, "HH:mm"); font.pixelSize: 30; font.weight: Font.DemiBold }
    Label { x: 649; y: 57; width: 123; horizontalAlignment: Text.AlignRight; text: Qt.formatDateTime(display.now, "dd MMM yyyy"); color: tokens.muted; font.pixelSize: 10 }
    Rule { x: 28; y: 85; width: 744 }


}
