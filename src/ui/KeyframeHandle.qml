import QtQuick
import QtQuick.Controls

Item {
    id: root

    property double timeSeconds: 0.0
    property double value: 1.0         // Normalized 0.0 to 1.0
    property string propertyName: "Opacity"
    property color handleColor: "#00E5FF"
    property bool isSelected: false
    property double pixelsPerSecond: 100.0
    property double trackHeight: 60.0

    signal keyframeMoved(double newTime, double newValue)
    signal keyframeSelected()

    width: 14
    height: 14
    x: (timeSeconds * pixelsPerSecond) - (width / 2)
    y: (trackHeight * (1.0 - value)) - (height / 2)

    // Diamond keyframe marker
    Rectangle {
        id: diamond
        anchors.centerIn: parent
        width: 10
        height: 10
        rotation: 45
        radius: 1
        color: dragArea.containsMouse || root.isSelected ? "#FFFFFF" : root.handleColor
        border.color: root.isSelected ? "#FFD600" : "#1A1D28"
        border.width: 1.5

        scale: dragArea.containsMouse || dragArea.drag.active ? 1.3 : 1.0
        Behavior on scale { NumberAnimation { duration: 100 } }
    }

    // Interactive Drag Handler
    MouseArea {
        id: dragArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        drag.target: root
        drag.axis: Drag.XAndYAxis
        drag.minimumX: -7
        drag.maximumX: 100000
        drag.minimumY: -7
        drag.maximumY: root.trackHeight - 7

        onPressed: {
            root.isSelected = true
            root.keyframeSelected()
        }

        onPositionChanged: {
            if (drag.active) {
                let newT = Math.max(0.0, (root.x + root.width / 2) / root.pixelsPerSecond)
                let newV = Math.max(0.0, Math.min(1.0, 1.0 - ((root.y + root.height / 2) / root.trackHeight)))
                root.timeSeconds = newT
                root.value = newV
                root.keyframeMoved(newT, newV)
            }
        }
    }

    // Tooltip HUD on Hover / Drag
    Rectangle {
        visible: dragArea.containsMouse || dragArea.drag.active
        anchors.bottom: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 6
        width: tipText.contentWidth + 12
        height: 18
        radius: 4
        color: "#E614161F"
        border.color: root.handleColor
        border.width: 1
        z: 100

        Text {
            id: tipText
            anchors.centerIn: parent
            text: root.propertyName + ": " + (root.value * 100).toFixed(0) + "% (" + root.timeSeconds.toFixed(2) + "s)"
            color: "#FFFFFF"
            font.pixelSize: 9
            font.bold: true
            font.family: "Monospace"
        }
    }
}
