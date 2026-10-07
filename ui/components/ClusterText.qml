import QtQuick

Text {
    required property QtObject tokens
    color: tokens.ink
    font.family: Qt.application.font.family
    font.pixelSize: 14
}
