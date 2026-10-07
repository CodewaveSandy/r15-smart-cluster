import QtQuick

// Desktop adapter only. Future physical input invokes shell.action() independently.
Item {
    required property QtObject shell
    required property QtObject simulator
    readonly property QtObject display: shell.display
    readonly property QtObject ride: shell.ride
    focus: true
        Keys.onPressed: function(event) {
            event.accepted = true
            if (event.key === Qt.Key_Escape) { shell.action("back"); return }
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) { shell.action("select"); return }
            if (event.key === Qt.Key_S && shell.screen === "dashboard") { shell.screen = "settings"; return }
            if (event.key === Qt.Key_F1) { shell.screen = shell.screen === "help" ? "dashboard" : "help"; return }
            if (shell.screen !== "dashboard") {
                if (shell.screen === "clock") {
                    if (event.key === Qt.Key_Up) shell.action("up")
                    else if (event.key === Qt.Key_Down) shell.action("down")
                    else if (event.key === Qt.Key_Right) shell.action("right")
                    else if (event.key === Qt.Key_Left) shell.action("left")
                } else {
                    if (event.key === Qt.Key_Down) shell.action("down")
                    if (event.key === Qt.Key_Up) shell.action("up")
                }
                return
            }
            switch (event.key) {
            case Qt.Key_Tab: display.track = !display.track; break
            case Qt.Key_PageDown: shell.action("next"); break
            case Qt.Key_PageUp: shell.action("previous"); break
            case Qt.Key_Right: simulator.adjustSpeed(1); break
            case Qt.Key_Left: simulator.adjustSpeed(-1); break
            case Qt.Key_Up: simulator.adjustRpm(100); break
            case Qt.Key_Down: simulator.adjustRpm(-100); break
            case Qt.Key_N: case Qt.Key_0: simulator.setGear(0); break
            case Qt.Key_1: case Qt.Key_2: case Qt.Key_3: case Qt.Key_4: case Qt.Key_5: case Qt.Key_6: simulator.setGear(event.key - Qt.Key_0); break
            case Qt.Key_L: simulator.toggleLeft(); break
            case Qt.Key_R: simulator.toggleRight(); break
            case Qt.Key_Z: simulator.toggleHazards(); break
            case Qt.Key_H: simulator.toggleHighBeam(); break
            case Qt.Key_E: simulator.toggleEngineWarning(); break
            case Qt.Key_Q: simulator.toggleQuickShifter(); break
            case Qt.Key_P: simulator.togglePhone(); break
            case Qt.Key_A: simulator.toggleAbs(); break
            case Qt.Key_T: simulator.toggleTcs(); break
            case Qt.Key_C: simulator.toggleCoolant(); break
            case Qt.Key_M: simulator.toggleMessage(); break
            case Qt.Key_I: simulator.toggleCall(); break
            case Qt.Key_K: if (display.track) ride.lapAction(); break
            case Qt.Key_B: if (display.track) ride.stopLap(); break
            case Qt.Key_BracketLeft: simulator.adjustFuel(-1); break
            case Qt.Key_BracketRight: simulator.adjustFuel(1); break
            case Qt.Key_Space: simulator.reset(); break
            default: event.accepted = false
            }
        }
}
