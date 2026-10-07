import QtQuick
import QtQuick.Window

// Independent startup fallback. No fonts, images or normal dashboard dependencies.
Window {
    width: 800; height: 480; visible: true; color: "#090e14"
    title: "R15 Smart Cluster — startup error"
    Text { anchors.centerIn: parent; width: 720; wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter
        color: "#ff7980"; font.pixelSize: 25
        text: "CLUSTER DISPLAY UNAVAILABLE\n\nStartup failed. Check diagnostic logs.\nNo live vehicle readings are being displayed." }
}
