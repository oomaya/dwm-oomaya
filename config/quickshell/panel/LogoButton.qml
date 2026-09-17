import QtQuick
import QtQuick.Layouts
import qs.core

PanelPill {
    id: root

    signal activated

    Layout.preferredWidth: Theme.pillHeight
    Layout.preferredHeight: Theme.pillHeight
    hovered: logoMouse.containsMouse

    Canvas {
        id: logoCanvas

        anchors.centerIn: parent
        width: 20
        height: 20

        property color strokeColor: root.hovered ? Theme.accentSecondary : Theme.accent

        onStrokeColorChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Component.onCompleted: requestPaint()

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            var s = Math.min(width, height) / 20.0;
            ctx.strokeStyle = strokeColor;
            ctx.lineWidth = 1.4 * s;
            ctx.lineCap = "square";
            ctx.lineJoin = "miter";

            // Outer Omarchy maze boundary with bottom and top ports
            ctx.beginPath();
            ctx.moveTo(11.25 * s, 18.75 * s);
            ctx.lineTo(1.25 * s, 18.75 * s);
            ctx.lineTo(1.25 * s, 1.25 * s);
            ctx.lineTo(18.75 * s, 1.25 * s);
            ctx.lineTo(18.75 * s, 18.75 * s);
            ctx.lineTo(8.75 * s, 18.75 * s);

            // Top port pin
            ctx.moveTo(10.0 * s, 1.25 * s);
            ctx.lineTo(10.0 * s, 3.75 * s);

            // Middle maze loop
            ctx.moveTo(11.0 * s, 3.75 * s);
            ctx.lineTo(3.75 * s, 3.75 * s);
            ctx.lineTo(3.75 * s, 16.25 * s);
            ctx.lineTo(16.25 * s, 16.25 * s);
            ctx.lineTo(16.25 * s, 3.75 * s);
            ctx.lineTo(14.5 * s, 3.75 * s);

            // Bottom port pin
            ctx.moveTo(10.0 * s, 18.75 * s);
            ctx.lineTo(10.0 * s, 16.25 * s);

            // Left connector
            ctx.moveTo(1.25 * s, 10.0 * s);
            ctx.lineTo(3.75 * s, 10.0 * s);
            ctx.stroke();

            // Inner DWM dynamic tiling core: Master (left) + Stack (right)
            ctx.beginPath();
            ctx.rect(6.25 * s, 6.25 * s, 7.5 * s, 7.5 * s);
            ctx.moveTo(10.0 * s, 6.25 * s);
            ctx.lineTo(10.0 * s, 13.75 * s);
            ctx.moveTo(10.0 * s, 10.0 * s);
            ctx.lineTo(13.75 * s, 10.0 * s);
            ctx.stroke();
        }
    }

    UiText {
        anchors.centerIn: parent
        visible: !logoCanvas.visible
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
