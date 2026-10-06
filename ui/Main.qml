import QtQuick
import QtQuick.Window

Window {
    id: window
    required property QtObject bike
    width: 800
    height: 480
    minimumWidth: 800
    maximumWidth: 800
    minimumHeight: 480
    maximumHeight: 480
    visible: true
    title: "R15 Smart Cluster · Desktop simulator"
    color: "#090e14"

    readonly property color ink: "#edf4fb"
    readonly property color muted: "#8194a8"
    readonly property color accent: "#78e4c1"
    property bool blink: true
    Timer { interval: 450; running: true; repeat: true; onTriggered: window.blink = !window.blink }

    Item {
        anchors.fill: parent
        focus: true
        Keys.onPressed: function(event) {
            switch (event.key) {
            case Qt.Key_Right: bike.adjustSpeed(5); break;
            case Qt.Key_Left: bike.adjustSpeed(-5); break;
            case Qt.Key_Up: bike.adjustRpm(500); break;
            case Qt.Key_Down: bike.adjustRpm(-500); break;
            case Qt.Key_0: case Qt.Key_N: bike.setGear(0); break;
            case Qt.Key_1: case Qt.Key_2: case Qt.Key_3:
            case Qt.Key_4: case Qt.Key_5: case Qt.Key_6: bike.setGear(event.key - Qt.Key_0); break;
            case Qt.Key_L: bike.toggleLeft(); break;
            case Qt.Key_R: bike.toggleRight(); break;
            case Qt.Key_H: bike.toggleHighBeam(); break;
            case Qt.Key_E: bike.toggleEngineWarning(); break;
            case Qt.Key_BracketLeft: bike.adjustFuel(-5); break;
            case Qt.Key_BracketRight: bike.adjustFuel(5); break;
            case Qt.Key_Space: bike.reset(); break;
            default: event.accepted = false; return;
            }
            event.accepted = true;
        }

        Text { x: 32; y: 30; text: "←"; color: bike.leftIndicator && window.blink ? window.accent : "#273442"; font.pixelSize: 34 }
        Text { x: 735; y: 30; text: "→"; color: bike.rightIndicator && window.blink ? window.accent : "#273442"; font.pixelSize: 34 }
        Row {
            x: 96; y: 42; spacing: 20
            Text { text: "HIGH BEAM"; color: bike.highBeam ? "#6cb9ff" : "#526275"; font.pixelSize: 11; font.bold: true }
            Text { text: "ENGINE"; color: bike.engineWarning ? "#ffbb5c" : "#526275"; font.pixelSize: 11; font.bold: true }
        }
        Text { x: 32; y: 147; text: "STREET"; color: window.accent; font.pixelSize: 12; font.letterSpacing: 3 }
        Text { x: 32; y: 170; text: "DESIGN 01"; color: window.muted; font.pixelSize: 10; font.letterSpacing: 2 }
        Text {
            x: 182; y: 112; width: 348; horizontalAlignment: Text.AlignHCenter
            text: bike.speed; color: window.ink; font.pixelSize: 132; font.weight: Font.Bold
            font.family: "Helvetica Neue"
        }
        Text { x: 182; y: 260; width: 348; horizontalAlignment: Text.AlignHCenter; text: "km/h"; color: window.muted; font.pixelSize: 17 }
        Rectangle {
            x: 604; y: 140; width: 132; height: 140; radius: 16; color: "#121d29"; border.color: "#243448"
            Text { anchors.horizontalCenter: parent.horizontalCenter; y: 14; text: "GEAR"; color: window.muted; font.pixelSize: 11; font.letterSpacing: 3 }
            Text { anchors.horizontalCenter: parent.horizontalCenter; y: 31; text: bike.gear === 0 ? "N" : bike.gear; color: bike.gear === 0 ? window.accent : window.ink; font.pixelSize: 72; font.weight: Font.DemiBold }
        }
        Row {
            x: 32; y: 305; spacing: 0
            Repeater {
                model: 13
                Text { required property int index; width: 58; text: index; color: window.muted; font.pixelSize: 10 }
            }
        }
        Rectangle {
            id: rpmTrack
            x: 32; y: 326; width: 736; height: 20
            color: "#1b2938"
            Item {
                width: rpmTrack.width * Math.max(0, Math.min(bike.rpm, 12000)) / 12000
                height: parent.height
                clip: true
                // Keep the gradient fixed to the RPM scale, revealing it as RPM rises.
                // Resizing the gradient itself would shift the color thresholds.
                Rectangle {
                    width: rpmTrack.width; height: rpmTrack.height
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: "#4cdb88" }
                        GradientStop { position: 3500 / 12000; color: "#4cdb88" }
                        GradientStop { position: 4500 / 12000; color: "#f5d45b" }
                        GradientStop { position: 6500 / 12000; color: "#f5d45b" }
                        GradientStop { position: 7500 / 12000; color: "#f06464" }
                        GradientStop { position: 1; color: "#f06464" }
                    }
                }
            }
        }
        Text { x: 32; y: 356; text: "RPM / 1,000"; color: window.muted; font.pixelSize: 10; font.letterSpacing: 1 }
        Text { x: 616; y: 353; width: 152; horizontalAlignment: Text.AlignRight; text: bike.rpm.toLocaleString(Qt.locale("en_US"), 'f', 0) + " rpm"; color: window.ink; font.pixelSize: 15 }
        Rectangle { x: 32; y: 389; width: 736; height: 1; color: "#24303e" }
        Text { x: 32; y: 405; text: "FUEL"; color: window.muted; font.pixelSize: 11; font.letterSpacing: 1 }
        Rectangle {
            x: 79; y: 408; width: 90; height: 8; radius: 4; color: "#24303e"
            Rectangle { width: parent.width * bike.fuel / 100; height: 8; radius: 4; color: bike.fuel <= 15 ? "#ffbb5c" : window.accent }
        }
        Text { x: 183; y: 402; text: bike.fuel + "%"; color: window.ink; font.pixelSize: 16 }
        Text { x: 300; y: 405; text: "SIMULATED DATA · NO BIKE CONNECTED"; color: window.muted; font.pixelSize: 10; font.letterSpacing: 1 }
        Text { x: 32; y: 448; text: "← → speed    ↑ ↓ rpm    1–6 gear    N neutral    L/R signals    H beam    E warning    [ ] fuel    Space reset"; color: "#8194a8"; font.pixelSize: 10 }
    }
}
