import QtQuick

Item {
    id: root
    required property int rpm
    required property bool track
    required property bool available
    required property QtObject tokens
    component Label: ClusterText { tokens: root.tokens }

    height: track ? 91 : 83
    Row {
        width: parent.width
        Repeater {
            model: 14
            Label { required property int index; width: root.width / 13; text: index; color: tokens.muted; font.pixelSize: 10 }
        }
    }
    Rectangle {
        id: rpmTrack
        y: 20; width: parent.width; height: track ? 52 : 44
        color: tokens.panel
        Item {
            width: rpmTrack.width * Math.max(0, Math.min(available ? rpm : 0, 13000)) / 13000
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
    Label { y: rpmTrack.y + rpmTrack.height + 6; text: "RPM × 1,000"; color: tokens.muted; font.pixelSize: 9; font.letterSpacing: 1 }
    Label { y: rpmTrack.y + rpmTrack.height + 4; anchors.right: parent.right; text: !available ? "— rpm" : rpm.toLocaleString(Qt.locale("en_US"), 'f', 0) + " rpm"; font.pixelSize: 13 }

}
