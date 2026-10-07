import QtQuick

Item {
    required property string kind
    required property string description
    property bool active: false
    required property color activeColor
    required property color inactiveColor
    readonly property real inactiveOpacity: 0.12
    width: 52; height: 25
    Accessible.role: Accessible.Indicator
    Accessible.name: description
    Accessible.description: active ? "Active" : "Inactive"
    ClusterIcon {
        objectName: "tellTaleIcon"
        anchors.centerIn: parent
        width: 26; height: 26
        kind: parent.kind
        tint: parent.active ? parent.activeColor : parent.inactiveColor
        opacity: parent.active ? 1 : parent.inactiveOpacity
    }
}
