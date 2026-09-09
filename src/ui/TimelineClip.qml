import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string clipId: "clip_1"
    property string clipName: "Video Layer"
    property string clipType: "video" // "video", "audio", "fx", "text"
    property double startTime: 0.0     // Seconds
    property double duration: 10.0      // Seconds
    property double inPoint: 0.0       // Seconds
    property double pixelsPerSecond: 100.0
    property bool isSelected: false
    property color clipColor: {
        if (clipType === "video") return "#54A0FF"
        if (clipType === "audio") return "#1DD1A1"
        if (clipType === "fx") return "#FF6E6A"
        if (clipType === "text") return "#FECA57"
        return "#48DBFB"
    }

    signal clipMoved(double newStartTime)
    signal clipTrimmed(double newStartTime, double newDuration)
    signal clipSelected()
    signal clipSplitRequested(string id)
    signal clipDeleteRequested(string id)
    signal clipDuplicateRequested(string id)

    x: startTime * pixelsPerSecond
    width: Math.max(30, duration * pixelsPerSecond)
    height: parent ? parent.height - 8 : 54
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    // Main Clip Body (Matching video frams.avif)
    Rectangle {
        id: body
        anchors.fill: parent
        radius: 8
        color: root.clipColor
        border.color: root.isSelected ? "#FFFFFF" : Qt.lighter(root.clipColor, 1.2)
        border.width: root.isSelected ? 2.5 : 1
        clip: true

        // Top Subtle Highlight
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: "#66FFFFFF"
        }

        // Clip Title & Type Icon
        RowLayout {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 10
            anchors.topMargin: 8
            spacing: 6
            z: 2

            Text {
                text: root.clipName
                color: (root.clipType === "text" || root.clipColor === "#FECA57") ? "#1E2235" : "#FFFFFF"
                font.pixelSize: 11
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                text: "[" + root.duration.toFixed(1) + "s]"
                color: (root.clipType === "text" || root.clipColor === "#FECA57") ? "#4A5568" : "#E2E8F0"
                font.pixelSize: 9
                font.family: "Monospace"
            }
        }

        // Inline Diamond Keyframe Badge (Matching video frams.avif)
        Rectangle {
            id: keyframeDiamond
            x: Math.min(parent.width - 40, Math.max(50, parent.width * 0.45))
            anchors.verticalCenter: parent.verticalCenter
            width: 10
            height: 10
            rotation: 45
            radius: 1
            color: "#FFFFFF"
            opacity: 0.9
            z: 5
        }

        // Waveform preview for audio clips
        Canvas {
            id: waveCanvas
            visible: root.clipType === "audio"
            anchors.fill: parent
            anchors.topMargin: 20
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = "#FFFFFF";
                ctx.lineWidth = 1.2;
                ctx.beginPath();
                var midY = height / 2;
                var step = 5;
                for (var x = 0; x < width; x += step) {
                    var amp = Math.sin(x * 0.08) * Math.cos(x * 0.03) * (height * 0.38);
                    ctx.moveTo(x, midY - amp);
                    ctx.lineTo(x, midY + amp);
                }
                ctx.stroke();
            }
        }

        // Video Filmstrip Thumbnails for video clips
        Row {
            visible: root.clipType === "video"
            anchors.fill: parent
            anchors.topMargin: 22
            spacing: 3
            clip: true
            opacity: 0.35

            Repeater {
                model: Math.max(1, Math.floor(root.width / 50))
                Rectangle {
                    width: 46
                    height: parent.height - 4
                    color: Qt.darker(root.clipColor, 1.4)
                    radius: 3
                }
            }
        }

        // Right Edge Grabber (≡ 3 horizontal lines from video frams.avif)
        Column {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 3
            z: 10

            Rectangle { width: 14; height: 1.5; color: "#40000000"; radius: 1 }
            Rectangle { width: 14; height: 1.5; color: "#40000000"; radius: 1 }
            Rectangle { width: 14; height: 1.5; color: "#40000000"; radius: 1 }
        }

        // Move Drag Handler (Horizontal)
        MouseArea {
            id: moveArea
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            cursorShape: Qt.SizeAllCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            drag.target: root
            drag.axis: Drag.XAxis
            drag.minimumX: 0

            onPressed: function(mouse) {
                root.isSelected = true
                root.clipSelected()

                if (mouse.button === Qt.RightButton) {
                    clipContextMenu.popup()
                }
            }

            onClicked: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    // Toggle left-click options pill
                    quickActionsPill.visible = !quickActionsPill.visible
                }
            }

            onPositionChanged: {
                if (drag.active) {
                    var newStart = Math.max(0.0, root.x / root.pixelsPerSecond)
                    root.startTime = newStart
                    root.clipMoved(newStart)
                }
            }
        }

        // Left Trim Handle
        Rectangle {
            id: leftHandle
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 8
            color: leftMouse.containsMouse || leftMouse.drag.active ? "#FFFFFF" : Qt.darker(root.clipColor, 1.3)
            opacity: 0.8
            radius: 4

            MouseArea {
                id: leftMouse
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                hoverEnabled: true

                property real startMouseX: 0
                property real origStartTime: 0
                property real origDuration: 0

                onPressed: {
                    startMouseX = mouse.x
                    origStartTime = root.startTime
                    origDuration = root.duration
                }

                onPositionChanged: {
                    if (pressed) {
                        var deltaSec = (mouse.x - startMouseX) / root.pixelsPerSecond
                        var newDur = Math.max(0.2, origDuration - deltaSec)
                        var newStart = Math.max(0.0, origStartTime + deltaSec)
                        root.startTime = newStart
                        root.duration = newDur
                        root.clipTrimmed(newStart, newDur)
                    }
                }
            }
        }

        // Right Trim Handle
        Rectangle {
            id: rightHandle
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 8
            color: rightMouse.containsMouse || rightMouse.drag.active ? "#FFFFFF" : Qt.darker(root.clipColor, 1.3)
            opacity: 0.8
            radius: 4

            MouseArea {
                id: rightMouse
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                hoverEnabled: true

                property real startMouseX: 0
                property real origDuration: 0

                onPressed: {
                    startMouseX = mouse.x
                    origDuration = root.duration
                }

                onPositionChanged: {
                    if (pressed) {
                        var deltaSec = (mouse.x - startMouseX) / root.pixelsPerSecond
                        var newDur = Math.max(0.2, origDuration + deltaSec)
                        root.duration = newDur
                        root.clipTrimmed(root.startTime, newDur)
                    }
                }
            }
        }
    }

    // ==================== LEFT-CLICK QUICK ACTIONS PILL ====================
    Rectangle {
        id: quickActionsPill
        visible: false
        anchors.bottom: body.top
        anchors.horizontalCenter: body.horizontalCenter
        anchors.bottomMargin: 6
        width: actionsRow.width + 12
        height: 28
        radius: 14
        color: "#1E2235"
        border.color: "#00E5FF"
        border.width: 1
        z: 100

        RowLayout {
            id: actionsRow
            anchors.centerIn: parent
            spacing: 6

            // Split / Cut Action
            Button {
                text: "✂ Split"
                Layout.preferredHeight: 22
                contentItem: Text { text: parent.text; color: "#00E5FF"; font.pixelSize: 10; font.bold: true }
                background: Rectangle { color: parent.hovered ? "#2C334D" : "transparent"; radius: 4 }
                onClicked: {
                    root.clipSplitRequested(root.clipId)
                    quickActionsPill.visible = false
                }
            }

            Rectangle { width: 1; height: 14; color: "#3A3F58" }

            // Duplicate Action
            Button {
                text: "📋 Duplicate"
                Layout.preferredHeight: 22
                contentItem: Text { text: parent.text; color: "#00E676"; font.pixelSize: 10; font.bold: true }
                background: Rectangle { color: parent.hovered ? "#2C334D" : "transparent"; radius: 4 }
                onClicked: {
                    root.clipDuplicateRequested(root.clipId)
                    quickActionsPill.visible = false
                }
            }

            Rectangle { width: 1; height: 14; color: "#3A3F58" }

            // Delete Action
            Button {
                text: "🗑 Delete"
                Layout.preferredHeight: 22
                contentItem: Text { text: parent.text; color: "#FF5252"; font.pixelSize: 10; font.bold: true }
                background: Rectangle { color: parent.hovered ? "#2C334D" : "transparent"; radius: 4 }
                onClicked: {
                    root.clipDeleteRequested(root.clipId)
                    quickActionsPill.visible = false
                }
            }
        }
    }

    // ==================== RIGHT-CLICK CONTEXT MENU ====================
    Menu {
        id: clipContextMenu

        MenuItem {
            text: "✂ Split Clip at Playhead"
            onTriggered: root.clipSplitRequested(root.clipId)
        }
        MenuItem {
            text: "📋 Duplicate Clip"
            onTriggered: root.clipDuplicateRequested(root.clipId)
        }
        MenuSeparator {}
        MenuItem {
            text: "🗑 Delete Clip"
            onTriggered: root.clipDeleteRequested(root.clipId)
        }
    }
}
