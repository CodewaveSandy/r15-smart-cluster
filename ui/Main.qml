import QtQuick
import QtQuick.Window
import "components"

Window {
    id: window
    required property QtObject bike
    required property QtObject display
    required property QtObject ride
    width: 800; height: 480
    minimumWidth: 800; maximumWidth: 800
    minimumHeight: 480; maximumHeight: 480
    visible: true
    title: "R15 Smart Cluster · Desktop simulator"
    readonly property bool shiftWarning: bike.rpm >= 10500
    readonly property bool shiftFlashing: bike.rpm >= 11400
    property bool shiftPulse: true
    readonly property bool shiftRed: shiftWarning && (!shiftFlashing || shiftPulse)
    readonly property bool lightPalette: display.light && !shiftRed
    color: display.light ? "#eff3f5" : "#090e14"
    Rectangle {
        anchors.fill: parent
        visible: window.shiftRed
        gradient: Gradient {
            GradientStop { position: 0; color: "#492731" }
            GradientStop { position: 0.55; color: "#b34c48" }
            GradientStop { position: 1; color: "#64303b" }
        }
    }
    Timer {
        interval: 300; running: window.shiftFlashing; repeat: true
        onTriggered: window.shiftPulse = !window.shiftPulse
    }
    onShiftFlashingChanged: shiftPulse = true
    readonly property color ink: lightPalette ? "#122333" : "#edf4fb"
    readonly property color muted: shiftRed ? "#f0d5d0" : lightPalette ? "#4d6577" : "#91a4b7"
    readonly property color panel: shiftRed ? "#352730" : lightPalette ? "#e0e8ec" : "#14202c"
    readonly property color line: shiftRed ? "#527f6268" : lightPalette ? "#c7d3db" : "#253544"
    readonly property color accent: lightPalette ? "#007757" : "#78e4c1"
    readonly property color amber: lightPalette ? "#946000" : "#f5d45b"
    readonly property color red: lightPalette ? "#bf263a" : "#ff7980"
    property string screen: "dashboard"
    property int selection: 0
    property int infoIndex: 0
    property bool blink: true
    property var clockDraft: [2026, 1, 1, 10, 0]
    property int clockField: 0
    property string clockError: ""
    readonly property var streetPages: ["ODO", "TRIP A", "TRIP B", "TRIP F", "BATTERY", "COOLANT", "AVG ECONOMY", "AVG SPEED", "TCS"]
    readonly property var trackPages: ["CURRENT LAP", "LATEST LAP", "FASTEST LAP", "ODO", "TRIP A", "TRIP B", "BATTERY", "COOLANT", "TCS"]
    readonly property var pages: display.track ? trackPages : streetPages
    readonly property string activePage: pages[infoIndex % pages.length]
    readonly property var menu: ["Display mode", "Theme", "Units", "Time & date", "Vehicle information", "Reset Trip A", "Reset Trip B", "Reset lap session", "Use Mac clock"]
    Timer { interval: 450; running: true; repeat: true; onTriggered: window.blink = !window.blink }
    Connections { target: display; function onChanged() { window.infoIndex = 0 } }

    component Label: Text { color: window.ink; font.family: Qt.application.font.family; font.pixelSize: 14 }
    component Rule: Rectangle { height: 1; color: window.line }
    component Lamp: Rectangle {
        property string label
        property bool active: false
        property color activeColor: window.amber
        width: lampText.implicitWidth + 18; height: 25; radius: 5
        color: active ? activeColor : window.panel
        Label { id: lampText; anchors.centerIn: parent; text: parent.label; color: parent.active ? (window.lightPalette ? "#ffffff" : "#101820") : window.muted; font.pixelSize: 10; font.bold: true }
    }
    function distance(km, decimals) { return (km * (display.imperial ? 0.621371 : 1)).toFixed(decimals) }
    function lap(ms) {
        if (ms <= 0) return "--:--.--"
        return Math.floor(ms / 60000).toString().padStart(2, "0") + ":" + Math.floor(ms / 1000 % 60).toString().padStart(2, "0") + "." + Math.floor(ms / 10 % 100).toString().padStart(2, "0")
    }
    function pageValue(name) {
        switch (name) {
        case "ODO": return distance(ride.odometer, 1)
        case "TRIP A": return distance(ride.tripA, 1)
        case "TRIP B": return distance(ride.tripB, 1)
        case "TRIP F": return bike.fuel <= 15 ? distance(ride.reserveTrip, 1) : "—"
        case "BATTERY": return "12.8"
        case "COOLANT": return bike.coolantWarning ? "115" : "84"
        case "AVG ECONOMY": return "—"
        case "AVG SPEED": return "—"
        case "TCS": return bike.tcsWarning ? "CHECK" : "—"
        case "CURRENT LAP": return lap(ride.currentLap)
        case "LATEST LAP": return lap(ride.latestLap)
        case "FASTEST LAP": return lap(ride.fastestLap)
        }
        return "—"
    }
    function pageUnit(name) {
        if (name === "BATTERY") return "V · bike"
        if (name === "COOLANT") return "°C"
        if (name === "AVG ECONOMY") return display.imperial ? "MPG · unavailable" : "km/L · unavailable"
        if (name === "AVG SPEED") return display.imperial ? "mph · unavailable" : "km/h · unavailable"
        if (name === "TCS") return "status unverified"
        if (name.indexOf("LAP") >= 0) return "min:sec · hundredths"
        return display.imperial ? "mi" : "km"
    }
    function openClock() {
        const date = display.now
        clockDraft = [date.getFullYear(), date.getMonth() + 1, date.getDate(), date.getHours(), date.getMinutes()]
        clockField = 0; clockError = ""; screen = "clock"
    }
    function adjustClock(delta) {
        if (clockField === 5) return
        let values = clockDraft.slice()
        const lower = [2020, 1, 1, 0, 0]
        const upper = [2099, 12, new Date(values[0], values[1], 0).getDate(), 23, 59]
        values[clockField] = Math.max(lower[clockField], Math.min(upper[clockField], values[clockField] + delta))
        values[2] = Math.min(values[2], new Date(values[0], values[1], 0).getDate())
        clockDraft = values; clockError = ""
    }
    function action(name) {
        if (name === "back") { screen = screen === "settings" ? "dashboard" : "settings"; return }
        if (screen === "dashboard") {
            if (name === "select" || name === "next") infoIndex = (infoIndex + 1) % pages.length
            if (name === "previous") infoIndex = (infoIndex + pages.length - 1) % pages.length
        } else if (screen === "settings") {
            if (name === "next") selection = (selection + 1) % menu.length
            if (name === "previous") selection = (selection + menu.length - 1) % menu.length
            if (name === "select") {
                switch (selection) {
                case 0: display.track = !display.track; break
                case 1: display.light = !display.light; break
                case 2: display.imperial = !display.imperial; break
                case 3: openClock(); break
                case 4: screen = "info"; break
                case 5: ride.resetTrip(0); break
                case 6: ride.resetTrip(1); break
                case 7: ride.resetLaps(); break
                case 8: display.useSystemClock(); break
                }
            }
        } else if (screen === "clock") {
            if (name === "next") clockField = (clockField + 1) % 6
            if (name === "previous") clockField = (clockField + 5) % 6
            if (name === "select") {
                if (clockField < 5) clockField++
                else if (display.setClock(clockDraft[0], clockDraft[1], clockDraft[2], clockDraft[3], clockDraft[4])) screen = "settings"
                else clockError = "Please choose a valid date and time."
            }
        }
    }

    Item {
        id: input
        anchors.fill: parent
        focus: true
        Keys.onPressed: function(event) {
            event.accepted = true
            if (event.key === Qt.Key_Escape) { window.action("back"); return }
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) { window.action("select"); return }
            if (event.key === Qt.Key_S && window.screen === "dashboard") { window.screen = "settings"; return }
            if (event.key === Qt.Key_F1) { window.screen = window.screen === "help" ? "dashboard" : "help"; return }
            if (window.screen !== "dashboard") {
                if (window.screen === "clock") {
                    if (event.key === Qt.Key_Up) window.adjustClock(1)
                    else if (event.key === Qt.Key_Down) window.adjustClock(-1)
                    else if (event.key === Qt.Key_Right) window.action("next")
                    else if (event.key === Qt.Key_Left) window.action("previous")
                } else {
                    if (event.key === Qt.Key_Down) window.action("next")
                    if (event.key === Qt.Key_Up) window.action("previous")
                }
                return
            }
            switch (event.key) {
            case Qt.Key_Tab: display.track = !display.track; break
            case Qt.Key_PageDown: window.action("next"); break
            case Qt.Key_PageUp: window.action("previous"); break
            case Qt.Key_Right: bike.adjustSpeed(1); break
            case Qt.Key_Left: bike.adjustSpeed(-1); break
            case Qt.Key_Up: bike.adjustRpm(100); break
            case Qt.Key_Down: bike.adjustRpm(-100); break
            case Qt.Key_N: case Qt.Key_0: bike.setGear(0); break
            case Qt.Key_1: case Qt.Key_2: case Qt.Key_3: case Qt.Key_4: case Qt.Key_5: case Qt.Key_6: bike.setGear(event.key - Qt.Key_0); break
            case Qt.Key_L: bike.toggleLeft(); break
            case Qt.Key_R: bike.toggleRight(); break
            case Qt.Key_Z: bike.toggleHazards(); break
            case Qt.Key_H: bike.toggleHighBeam(); break
            case Qt.Key_E: bike.toggleEngineWarning(); break
            case Qt.Key_Q: bike.toggleQuickShifter(); break
            case Qt.Key_P: bike.togglePhone(); break
            case Qt.Key_A: bike.toggleAbs(); break
            case Qt.Key_T: bike.toggleTcs(); break
            case Qt.Key_C: bike.toggleCoolant(); break
            case Qt.Key_M: bike.toggleMessage(); break
            case Qt.Key_I: bike.toggleCall(); break
            case Qt.Key_K: if (display.track) ride.lapAction(); break
            case Qt.Key_B: if (display.track) ride.stopLap(); break
            case Qt.Key_BracketLeft: bike.adjustFuel(-1); break
            case Qt.Key_BracketRight: bike.adjustFuel(1); break
            case Qt.Key_Space: bike.reset(); break
            default: event.accepted = false
            }
        }

        // Gear is always the first element, followed by mode and OEM status information.
        Label { x: 28; y: 19; text: "GEAR"; color: window.muted; font.pixelSize: 9; font.letterSpacing: 2 }
        Label { x: 28; y: 30; text: bike.gear === 0 ? "N" : bike.gear; color: bike.gear === 0 ? window.accent : window.ink; font.pixelSize: 46; font.bold: true }
        Rectangle { x: 96; y: 22; width: 1; height: 48; color: window.line }
        Label { x: 120; y: 26; text: display.track ? "TRACK" : "STREET"; font.pixelSize: 22; font.bold: true; font.letterSpacing: 2 }
        Label { x: 121; y: 55; text: display.track ? "LAP / PERFORMANCE" : "ROAD / EVERYDAY"; font.pixelSize: 9; color: window.muted; font.letterSpacing: 1 }
        Lamp { objectName: "qsLamp"; x: 288; y: 34; label: "QS"; visible: bike.quickShifter; active: true; activeColor: window.accent }
        Lamp { objectName: "vvaLamp"; x: 344; y: 34; label: "VVA"; visible: bike.vva; active: true; activeColor: window.amber }
        TellTale { x: 448; y: 34; kind: "phone"; description: "Phone connection"; active: bike.phoneConnected; activeColor: window.accent; inactiveColor: window.muted }
        Rectangle {
            x: 508; y: 37; width: 33; height: 17; radius: 2; color: "transparent"; border.color: bike.phoneConnected ? window.accent : window.muted
            Rectangle { x: 34; y: 5; width: 3; height: 7; color: window.muted }
            Rectangle { x: 3; y: 3; height: 11; width: bike.phoneConnected ? 27 * bike.phoneBattery / 100 : 0; color: window.accent }
        }
        Label { x: 551; y: 37; text: bike.phoneConnected ? bike.phoneBattery + "%" : "—"; color: window.muted; font.pixelSize: 13 }
        Label { x: 658; y: 23; width: 114; horizontalAlignment: Text.AlignRight; text: Qt.formatDateTime(display.now, "HH:mm"); font.pixelSize: 30; font.weight: Font.DemiBold }
        Label { x: 649; y: 57; width: 123; horizontalAlignment: Text.AlignRight; text: Qt.formatDateTime(display.now, "dd MMM yyyy"); color: window.muted; font.pixelSize: 10 }
        Rule { x: 28; y: 85; width: 744 }

        Item {
            visible: window.screen === "dashboard"
            anchors.fill: parent
            TellTale { objectName: "leftSignal"; x: 28; y: 98; width: 40; kind: "left"; description: "Left indicator"; active: bike.leftIndicator && window.blink; activeColor: window.accent; inactiveColor: window.muted }
            TellTale { x: 90; y: 98; width: 52; kind: "neutral"; description: "Neutral"; active: bike.gear === 0; activeColor: window.accent; inactiveColor: window.muted }
            TellTale { x: 161; y: 98; width: 52; kind: "beam"; description: "High beam"; active: bike.highBeam; activeColor: window.lightPalette ? "#246bb0" : "#77baff"; inactiveColor: window.muted }
            TellTale { x: 232; y: 98; width: 52; kind: "tcs"; description: "Traction control"; active: bike.tcsWarning; activeColor: window.amber; inactiveColor: window.muted }
            TellTale { x: 303; y: 98; width: 52; kind: "abs"; description: "ABS warning"; active: bike.absWarning; activeColor: window.amber; inactiveColor: window.muted }
            TellTale { x: 374; y: 98; width: 52; kind: "engine"; description: "Engine warning"; active: bike.engineWarning; activeColor: window.amber; inactiveColor: window.muted }
            TellTale { x: 445; y: 98; width: 52; kind: "temp"; description: "Coolant warning"; active: bike.coolantWarning; activeColor: window.red; inactiveColor: window.muted }
            TellTale { x: 516; y: 98; width: 52; kind: "message"; description: "Message"; active: bike.messagePending; activeColor: window.accent; inactiveColor: window.muted }
            TellTale { x: 587; y: 98; width: 52; kind: "call"; description: "Incoming call"; active: bike.incomingCall; activeColor: window.accent; inactiveColor: window.muted }
            TellTale { x: 658; y: 98; width: 52; kind: "hazard"; description: "Hazards"; active: bike.hazards && window.blink; activeColor: window.red; inactiveColor: window.muted }
            TellTale { objectName: "rightSignal"; x: 732; y: 98; width: 40; kind: "right"; description: "Right indicator"; active: bike.rightIndicator && window.blink; activeColor: window.accent; inactiveColor: window.muted }
            // The larger track tachometer retains the same color thresholds as Street.
            Item {
                id: tach
                x: 28; y: 144; width: 744; height: display.track ? 91 : 83
                Row {
                    width: parent.width
                    Repeater {
                        model: 14
                        Label { required property int index; width: tach.width / 13; text: index; color: window.muted; font.pixelSize: 10 }
                    }
                }
                Rectangle {
                    id: rpmTrack
                    y: 20; width: parent.width; height: display.track ? 52 : 44
                    color: window.panel
                    Item {
                        width: rpmTrack.width * Math.max(0, Math.min(bike.rpm, 13000)) / 13000
                        height: parent.height; clip: true
                        Rectangle {
                            width: rpmTrack.width; height: rpmTrack.height
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0; color: "#4cdb88" }
                                GradientStop { position: 3500 / 13000; color: "#4cdb88" }
                                GradientStop { position: 4500 / 13000; color: "#f5d45b" }
                                GradientStop { position: 7500 / 13000; color: "#f5d45b" }
                                GradientStop { position: 8500 / 13000; color: "#f06464" }
                                GradientStop { position: 1; color: "#f06464" }
                            }
                        }
                    }
                }
                Label { y: rpmTrack.y + rpmTrack.height + 6; text: "RPM × 1,000"; color: window.muted; font.pixelSize: 9; font.letterSpacing: 1 }
                Label { y: rpmTrack.y + rpmTrack.height + 4; anchors.right: parent.right; text: bike.rpm.toLocaleString(Qt.locale("en_US"), 'f', 0) + " rpm"; font.pixelSize: 13 }
            }
            Label {
                objectName: "speedValue"
                x: display.track ? 28 : 210; y: display.track ? 231 : 220
                width: display.track ? 255 : 380; horizontalAlignment: display.track ? Text.AlignLeft : Text.AlignHCenter
                text: Math.round(bike.speed * (display.imperial ? 0.621371 : 1)); font.pixelSize: display.track ? 79 : 128; font.weight: Font.Bold
            }
            Label { x: display.track ? 225 : 590; y: display.track ? 277 : 306; text: display.imperial ? "mph" : "km/h"; color: window.muted; font.pixelSize: 16 }
            Item {
                visible: display.track
                x: 365; y: 239
                Label { text: "LAP " + ride.lapNumber.toString().padStart(2, "0") + (ride.lapRunning ? "  /  RUNNING" : ride.currentLap > 0 ? "  /  STOPPED" : "  /  READY"); color: window.accent; font.pixelSize: 10; font.letterSpacing: 2 }
                Label { y: 20; text: window.lap(ride.currentLap); font.pixelSize: 42; font.weight: Font.DemiBold }
                Label { y: 72; text: "K  start / next lap     B  stop"; color: window.muted; font.pixelSize: 10 }
            }
            Rule { x: 28; y: display.track ? 336 : 376; width: 744 }
            Label { x: 28; y: display.track ? 351 : 386; text: window.activePage; color: window.accent; font.pixelSize: 10; font.letterSpacing: 2 }
            Label { x: 28; y: display.track ? 367 : 399; text: window.pageValue(window.activePage); font.pixelSize: display.track ? 31 : 26; font.weight: Font.DemiBold }
            Label { x: display.track ? 29 : 203; y: display.track ? 408 : 411; text: window.pageUnit(window.activePage); color: window.muted; font.pixelSize: 10 }
            Label { x: 267; y: display.track ? 351 : 386; text: display.track ? "FASTEST LAP" : "BIKE / COOLANT"; color: window.muted; font.pixelSize: 10; font.letterSpacing: 1 }
            Label { x: 267; y: display.track ? 377 : 402; text: display.track ? window.lap(ride.fastestLap) : "12.8 V  /  " + (bike.coolantWarning ? "115" : "84") + "°C"; font.pixelSize: 23 }
            Label { visible: display.track; x: 267; y: 409; text: display.track ? "LATEST  " + window.lap(ride.latestLap) : "Enter  cycle information"; color: window.muted; font.pixelSize: 10 }
            Rectangle { x: 537; y: display.track ? 353 : 387; width: 1; height: display.track ? 65 : 40; color: window.line }
            FuelGauge {
                objectName: "fuelGauge"
                x: 552; y: display.track ? 348 : 383; width: 220; height: display.track ? 80 : 55
                compact: !display.track
                fuelPercent: bike.fuel
                foreground: window.ink; muted: window.muted; depleted: window.panel
                accent: window.accent; warning: window.amber; critical: window.red
                fontFamily: Qt.application.font.family
            }
        }

        Item {
            visible: window.screen === "settings"
            x: 28; y: 104; width: 744; height: 321
            Label { text: "CONFIGURATION"; font.pixelSize: 18; font.bold: true; font.letterSpacing: 2 }
            Label { x: 425; y: 5; text: "↑ ↓ navigate   Enter select   Esc back"; color: window.muted; font.pixelSize: 11 }
            Column {
                y: 36; spacing: 2
                Repeater {
                    model: window.menu
                    Rectangle {
                        required property int index
                        required property string modelData
                        width: 744; height: 29; radius: 4
                        color: window.selection === index ? window.panel : "transparent"
                        border.color: window.selection === index ? window.accent : "transparent"
                        Label { x: 12; anchors.verticalCenter: parent.verticalCenter; text: (window.selection === parent.index ? "›  " : "   ") + parent.modelData; font.pixelSize: 13 }
                        Label {
                            x: 405; width: 320; anchors.verticalCenter: parent.verticalCenter; horizontalAlignment: Text.AlignRight; color: window.accent; font.pixelSize: 12
                            text: {
                                switch (parent.index) {
                                case 0: return display.track ? "Track" : "Street"
                                case 1: return display.light ? "Daylight" : "Midnight"
                                case 2: return display.imperial ? "mph / miles" : "km/h / km"
                                case 3: return Qt.formatDateTime(display.now, "dd MMM yyyy · HH:mm")
                                case 4: return "R15 V4 · 2026  ›"
                                case 5: return window.distance(ride.tripA, 1) + "  →  0"
                                case 6: return window.distance(ride.tripB, 1) + "  →  0"
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
        Item {
            visible: window.screen === "clock"
            x: 28; y: 112
            Label { text: "TIME & DATE"; font.pixelSize: 22; font.bold: true }
            Label { y: 43; text: "← → choose field   ↑ ↓ adjust   Enter next / save   Esc cancel"; color: window.muted; font.pixelSize: 13 }
            Row {
                y: 96; spacing: 12
                Repeater {
                    model: ["YEAR", "MONTH", "DAY", "HOUR", "MINUTE"]
                    Rectangle {
                        required property int index
                        required property string modelData
                        width: 139; height: 93; radius: 8; color: window.panel
                        border.color: window.clockField === index ? window.accent : window.line
                        Label { anchors.horizontalCenter: parent.horizontalCenter; y: 12; text: parent.modelData; color: window.muted; font.pixelSize: 10 }
                        Label { anchors.horizontalCenter: parent.horizontalCenter; y: 33; text: window.clockDraft[parent.index].toString().padStart(2, "0"); font.pixelSize: 30 }
                    }
                }
            }
            Rectangle { y: 219; width: 200; height: 40; radius: 6; color: window.panel; border.color: window.clockField === 5 ? window.accent : window.line
                Label { anchors.centerIn: parent; text: "SAVE DATE & TIME"; font.pixelSize: 12; color: window.accent }
            }
            Label { x: 226; y: 231; text: window.clockError || "Updates the cluster clock only."; color: window.clockError ? window.red : window.muted; font.pixelSize: 12 }
        }
        Item {
            visible: window.screen === "info"
            x: 28; y: 110
            Label { text: "VEHICLE / AT A GLANCE"; font.pixelSize: 20; font.bold: true; font.letterSpacing: 1 }
            Column {
                y: 46; spacing: 18
                Repeater {
                    model: [
                        {label: "MODEL", value: "Yamaha R15 V4 · 2026"},
                        {label: "DATA SOURCE", value: "Desktop simulation · no vehicle connected"},
                        {label: "ODOMETER", value: window.distance(ride.odometer, 1) + (display.imperial ? " mi" : " km") + " · demo reading"},
                        {label: "BIKE BATTERY", value: "12.8 V · sample"},
                        {label: "COOLANT", value: (bike.coolantWarning ? "115" : "84") + "°C · sample"},
                        {label: "PHONE", value: bike.phoneConnected ? "App connected · " + bike.phoneBattery + "% phone battery · simulated" : "Disconnected"},
                        {label: "QS / VVA", value: (bike.quickShifter ? "QS enabled" : "QS disabled") + " / " + (bike.vva ? "VVA active" : "VVA inactive")},
                        {label: "VIN / ECU", value: "Not available until vehicle integration"}
                    ]
                    Item {
                        required property var modelData
                        width: 744; height: 17
                        Label { text: parent.modelData.label; color: window.muted; font.pixelSize: 12 }
                        Label { x: 170; text: parent.modelData.value; font.pixelSize: 14 }
                    }
                }
            }
        }
        Item {
            visible: window.screen === "help"
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
        Rule { x: 28; y: 439; width: 744 }
        Label { x: 28; y: 452; text: window.screen === "dashboard" ? "Tab  mode    Enter  info    S  settings    Z  hazards    F1  controls" : "Esc  back     F1  controls"; color: window.muted; font.pixelSize: 10 }
        Label { x: 580; y: 452; width: 192; horizontalAlignment: Text.AlignRight; text: "SIMULATED · NO BIKE CONNECTED"; color: window.muted; font.pixelSize: 9 }
    }
}
