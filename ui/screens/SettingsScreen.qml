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

    x: 28; y: 104; width: 744; height: 321
    Label { text: "CONFIGURATION"; font.pixelSize: 18; font.bold: true; font.letterSpacing: 2 }
    Label { x: 425; y: 5; text: "↑ ↓ navigate   Enter select   Esc back"; color: tokens.muted; font.pixelSize: 11 }
    Column {
        y: 36; spacing: 2
        Repeater {
            model: shell.menu
            Rectangle {
                required property int index
                required property string modelData
                width: 744; height: 29; radius: 4
                color: shell.selection === index ? tokens.panel : "transparent"
                border.color: shell.selection === index ? tokens.accent : "transparent"
                Label { x: 12; anchors.verticalCenter: parent.verticalCenter; text: (shell.selection === parent.index ? "›  " : "   ") + parent.modelData; font.pixelSize: 13 }
                Label {
                    x: 405; width: 320; anchors.verticalCenter: parent.verticalCenter; horizontalAlignment: Text.AlignRight; color: tokens.accent; font.pixelSize: 12
                    text: {
                        switch (parent.index) {
                        case 0: return display.track ? "Track" : "Street"
                        case 1: return display.light ? "Daylight" : "Midnight"
                        case 2: return display.imperial ? "mph / miles" : "km/h / km"
                        case 3: return Qt.formatDateTime(display.now, "dd MMM yyyy · HH:mm")
                        case 4: return "R15 V4 · 2026  ›"
                        case 5: return shell.distance(ride.tripAKm, 1) + "  →  0"
                        case 6: return shell.distance(ride.tripBKm, 1) + "  →  0"
                        case 7: return "Clear session laps"
                        case 8: return "Restore automatic time"
                        }
                        return ""
                    }
                }
            }
        }
    }

}
