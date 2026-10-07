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

    x: 28; y: 112
    Label { text: "TIME & DATE"; font.pixelSize: 22; font.bold: true }
    Label { y: 43; text: "← → choose field   ↑ ↓ adjust   Enter next / save   Esc cancel"; color: tokens.muted; font.pixelSize: 13 }
    Row {
        y: 96; spacing: 12
        Repeater {
            model: ["YEAR", "MONTH", "DAY", "HOUR", "MINUTE"]
            Rectangle {
                required property int index
                required property string modelData
                width: 139; height: 93; radius: 8; color: tokens.panel
                border.color: shell.clockField === index ? tokens.accent : tokens.line
                Label { anchors.horizontalCenter: parent.horizontalCenter; y: 12; text: parent.modelData; color: tokens.muted; font.pixelSize: 10 }
                Label { anchors.horizontalCenter: parent.horizontalCenter; y: 33; text: shell.clockDraft[parent.index].toString().padStart(2, "0"); font.pixelSize: 30 }
            }
        }
    }
    Rectangle { y: 219; width: 200; height: 40; radius: 6; color: tokens.panel; border.color: shell.clockField === 5 ? tokens.accent : tokens.line
        Label { anchors.centerIn: parent; text: "SAVE DATE & TIME"; font.pixelSize: 12; color: tokens.accent }
    }
    Label { x: 226; y: 231; text: shell.clockError || "Updates the cluster clock only."; color: shell.clockError ? tokens.red : tokens.muted; font.pixelSize: 12 }

}
