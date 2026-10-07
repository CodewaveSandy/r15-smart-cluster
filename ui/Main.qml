import QtQuick
import QtQuick.Window
import "components"
import "screens"
import "theme"
import "input"

Window {
    id: window
    required property QtObject bike
    required property QtObject display
    required property QtObject ride
    required property QtObject simulator
    required property QtObject diagnostics
    width: 800; height: 480
    minimumWidth: 800; maximumWidth: 800
    minimumHeight: 480; maximumHeight: 480
    visible: true
    title: "R15 Smart Cluster · Desktop simulator"
    readonly property bool shiftWarning: bike.available && bike.engineRpm >= theme.shiftWarningRpm
    readonly property bool shiftFlashing: bike.available && bike.engineRpm >= theme.shiftFlashRpm
    property bool shiftPulse: true
    readonly property bool shiftRed: shiftWarning && (!shiftFlashing || shiftPulse)
    readonly property bool lightPalette: theme.lightPalette
    color: theme.background
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
        interval: theme.shiftFlashIntervalMs; running: window.shiftFlashing; repeat: true
        onTriggered: window.shiftPulse = !window.shiftPulse
    }
    onShiftFlashingChanged: shiftPulse = true
    readonly property color ink: theme.ink
    readonly property color muted: theme.muted
    readonly property color panel: theme.panel
    readonly property color line: theme.line
    readonly property color accent: theme.accent
    readonly property color amber: theme.amber
    readonly property color red: theme.red
    Theme { id: theme; light: display.light; shiftRed: window.shiftRed }
    property string screen: "dashboard"
    onScreenChanged: {
        if (["dashboard", "settings", "clock", "info", "help"].indexOf(screen) < 0) {
            screen = "dashboard"
            diagnostics.reportUiError("Rejected unknown screen")
        }
    }
    property int selection: 0
    property int infoIndex: 0
    property bool blink: true
    property var clockDraft: [2026, 1, 1, 10, 0]
    property int clockField: 0
    property string clockError: ""
    readonly property var streetPages: ["ODO", "TRIP A", "TRIP B", "TRIP F", "BATTERY", "COOLANT", "AVG ECONOMY", "AVG SPEED", "TCS"]
    readonly property var trackPages: ["CURRENT LAP", "LATEST LAP", "FASTEST LAP", "ODO", "TRIP A", "TRIP B", "BATTERY", "COOLANT", "TCS"]
    readonly property var pages: display.track ? trackPages : streetPages
    readonly property string activePage: pages[((infoIndex % pages.length) + pages.length) % pages.length]
    readonly property var menu: ["Display mode", "Theme", "Units", "Time & date", "Vehicle information", "Reset Trip A", "Reset Trip B", "Reset lap session", "Use Mac clock"]
    Timer { interval: theme.indicatorBlinkIntervalMs; running: true; repeat: true; onTriggered: window.blink = !window.blink }
    Connections { target: display; function onChanged() { window.infoIndex = 0 } }

    function distance(km, decimals) { return (km * (display.imperial ? 0.621371 : 1)).toFixed(decimals) }
    function lap(ms) {
        if (ms <= 0) return "--:--.--"
        return Math.floor(ms / 60000).toString().padStart(2, "0") + ":" + Math.floor(ms / 1000 % 60).toString().padStart(2, "0") + "." + Math.floor(ms / 10 % 100).toString().padStart(2, "0")
    }
    function pageValue(name) {
        if (!bike.available) return "—"
        switch (name) {
        case "ODO": return distance(ride.odometerKm, 1)
        case "TRIP A": return distance(ride.tripAKm, 1)
        case "TRIP B": return distance(ride.tripBKm, 1)
        case "TRIP F": return bike.fuelPercent <= 15 ? distance(ride.reserveTripKm, 1) : "—"
        case "BATTERY": return bike.batteryVoltageAvailable ? bike.batteryVoltageV.toFixed(1) : "—"
        case "COOLANT": return bike.coolantTemperatureAvailable ? Math.round(bike.coolantTemperatureC).toString() : "—"
        case "AVG ECONOMY": return "—"
        case "AVG SPEED": return "—"
        case "TCS": return bike.tcsWarning ? "CHECK" : "—"
        case "CURRENT LAP": return lap(ride.currentLapMs)
        case "LATEST LAP": return lap(ride.latestLapMs)
        case "FASTEST LAP": return lap(ride.fastestLapMs)
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
        if (["back", "select", "next", "previous", "up", "down", "left", "right"].indexOf(name) < 0) {
            diagnostics.reportUiError("Rejected unknown navigation action")
            return
        }
        if (name === "up" || name === "down") {
            if (screen === "clock") { adjustClock(name === "up" ? 1 : -1); return }
            name = name === "up" ? "previous" : "next"
        }
        if (name === "left" || name === "right") {
            if (screen !== "clock" && screen !== "dashboard") return
            name = name === "left" ? "previous" : "next"
        }
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

    DesktopInput { anchors.fill: parent; shell: window; simulator: window.simulator }
    ClusterHeader { anchors.fill: parent; bike: window.bike; display: window.display; tokens: theme }
    DashboardScreen { anchors.fill: parent; visible: window.screen === "dashboard"; shell: window; tokens: theme }
    SettingsScreen { visible: window.screen === "settings"; shell: window; tokens: theme }
    ClockScreen { visible: window.screen === "clock"; shell: window; tokens: theme }
    VehicleInfoScreen { visible: window.screen === "info"; shell: window; tokens: theme }
    HelpScreen { visible: window.screen === "help"; shell: window; tokens: theme }
    Rectangle { x: 28; y: 439; width: 744; height: 1; color: theme.line }
    ClusterText { tokens: theme; x: 28; y: 452; text: window.screen === "dashboard" ? "Tab  mode    Enter  info    S  settings    Z  hazards    F1  controls" : "Esc  back     F1  controls"; color: theme.muted; font.pixelSize: 10 }
    ClusterText { tokens: theme; x: 470; y: 452; width: 302; horizontalAlignment: Text.AlignRight
        text: diagnostics.uiError || display.persistenceError || (!bike.available ? "DATA UNAVAILABLE · " + bike.dataStatus : "SIMULATED · NO BIKE CONNECTED")
        color: diagnostics.uiError || display.persistenceError || !bike.available ? theme.red : theme.muted; font.pixelSize: 9 }
}
