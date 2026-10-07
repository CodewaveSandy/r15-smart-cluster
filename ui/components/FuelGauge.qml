import QtQuick

Item {
    id: gauge
    required property int fuelPercent
    required property color foreground
    required property color muted
    required property color depleted
    required property color accent
    required property color warning
    required property color critical
    required property string fontFamily
    property bool available: true
    property bool compact: false
    readonly property int lowFuelThresholdPercent: 20
    readonly property int blinkIntervalMs: 500
    readonly property bool lowFuel: available && fuelPercent < lowFuelThresholdPercent
    readonly property int criticalFuelThresholdPercent: 10
    readonly property bool criticalFuel: available && fuelPercent < criticalFuelThresholdPercent
    readonly property color statusColor: criticalFuel ? critical : available && fuelPercent <= lowFuelThresholdPercent ? warning : accent
    readonly property int filledSegments: available ? Math.ceil(Math.max(0, Math.min(100, fuelPercent)) / 12.5) : 0
    property bool pulse: true
    readonly property bool warningLit: !lowFuel || pulse
    onLowFuelChanged: pulse = true
    Timer {
        interval: gauge.blinkIntervalMs
        running: gauge.lowFuel && gauge.visible
        repeat: true
        onTriggered: gauge.pulse = !gauge.pulse
    }
    onVisibleChanged: pulse = true
    Text { x: 31; y: 3; text: "FUEL"; font.family: gauge.fontFamily; font.pixelSize: 10; font.letterSpacing: 2; color: gauge.muted }
    Text {
        anchors.right: parent.right; y: -4
        text: gauge.available ? gauge.fuelPercent + "%" : "—"; font.family: gauge.fontFamily; font.pixelSize: 23
        color: gauge.available && gauge.fuelPercent <= gauge.lowFuelThresholdPercent ? gauge.statusColor : gauge.foreground
    }
    ClusterIcon {
        x: 0; y: gauge.compact ? 18 : 27; width: 24; height: 26; kind: "fuel"
        tint: gauge.available && gauge.fuelPercent <= gauge.lowFuelThresholdPercent ? gauge.statusColor : gauge.foreground
        opacity: gauge.warningLit ? 1 : 0
    }
    Row {
        id: segments
        x: 31; y: gauge.compact ? 27 : 35
        width: gauge.width - x; height: gauge.compact ? 10 : 13; spacing: 2
        Repeater {
            model: 8
            Rectangle {
                required property int index
                width: (segments.width - segments.spacing * 7) / 8; height: segments.height
                color: index < gauge.filledSegments && gauge.warningLit ? gauge.statusColor : gauge.depleted
                transform: Matrix4x4 { matrix: Qt.matrix4x4(1,-0.3,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1) }
            }
        }
    }
    Text { x: 31; y: segments.y + segments.height + 2; text: "E"; font.family: gauge.fontFamily; font.pixelSize: 10; color: gauge.muted }
    Text { anchors.right: parent.right; y: segments.y + segments.height + 2; text: "F"; font.family: gauge.fontFamily; font.pixelSize: 10; color: gauge.muted }
}
