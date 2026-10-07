import QtQuick

QtObject {
    required property bool light
    required property bool shiftRed
    readonly property bool lightPalette: light && !shiftRed
    readonly property color background: light ? "#eff3f5" : "#090e14"
    readonly property color ink: lightPalette ? "#122333" : "#edf4fb"
    readonly property color muted: shiftRed ? "#f0d5d0" : lightPalette ? "#4d6577" : "#91a4b7"
    readonly property color panel: shiftRed ? "#352730" : lightPalette ? "#e0e8ec" : "#14202c"
    readonly property color line: shiftRed ? "#527f6268" : lightPalette ? "#c7d3db" : "#253544"
    readonly property color accent: lightPalette ? "#007757" : "#78e4c1"
    readonly property color amber: lightPalette ? "#946000" : "#f5d45b"
    readonly property color red: lightPalette ? "#bf263a" : "#ff7980"
    readonly property color beam: lightPalette ? "#246bb0" : "#77baff"
    readonly property int shiftWarningRpm: 10500
    readonly property int shiftFlashRpm: 11400
    readonly property int shiftFlashIntervalMs: 300
    readonly property int indicatorBlinkIntervalMs: 450
}
