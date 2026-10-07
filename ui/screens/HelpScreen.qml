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
    Label { text: "SIMULATOR CONTROLS"; font.pixelSize: 20; font.bold: true }
    Column {
        y: 48; spacing: 17
        Repeater {
            model: ["← / →    speed ±1 km/h       ↑ / ↓    RPM ±100       1–6 / N    gear / neutral",
                    "Tab    Street / Track       Enter or PgUp / PgDn    information pages",
                    "S    settings       F1    show / hide this guide       Esc    back",
                    "Q    quick shifter       P    app connection       M / I    message / call",
                    "L / R    exclusive turn signals       Z    hazards       H    beam       E    engine",
                    "A    ABS       T    TCS       C    coolant warning       [ / ]    fuel",
                    "K    start / record lap       B    stop lap       (Track layout only)",
                    "Space    reset bike signals       Trips / laps reset separately in settings"]
            Label { required property string modelData; text: modelData; font.pixelSize: 14 }
        }
    }

}
