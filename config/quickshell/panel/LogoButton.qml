import QtQuick
import QtQuick.Layouts
import qs.core

PanelPill {
    id: root

    signal activated

    Layout.preferredWidth: Theme.pillHeight
    Layout.preferredHeight: Theme.pillHeight
    hovered: logoMouse.containsMouse

    Image {
        id: logoImage

        anchors.centerIn: parent
        width: 20
        height: 20

        source: Qt.resolvedUrl("../assets/dwm_oomaya_logo.png")
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
        mipmap: true
        opacity: root.hovered ? 0.75 : 1.0

        Behavior on opacity {
            NumberAnimation { duration: 120 }
        }
    }

    UiText {
        anchors.centerIn: parent
        visible: logoImage.status === Image.Error
        text: "OMY"
        color: Theme.accent
        font.pixelSize: Theme.tinyFontSize
        font.bold: true
    }

    MouseArea {
        id: logoMouse

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.activated()
    }
}
