import QtQuick
import QtQuick.Controls

Item {
    id: root

    property string clipId: "clip_1"
    property string clipName: "Video Stream"
    property string clipType: "video" // "video", "audio", "fx"
    property double startTime: 0.0     // Seconds
    property double duration: 10.0      // Seconds
    property double inPoint: 0.0       // Seconds
    property double pixelsPerSecond: 100.0
    property bool isSelected: false
    property color clipColor: clipType === "video" ? "#1E3A5F" : (clipType === "audio" ? "#1B4D3E" : "#4A2868")
    property color accentColor: clipType === "video" ? "#00E5FF" : (clipType === "audio" ? "#00E676" : "#E040FB")

    signal clipMoved(double newStartTime)
    signal clipTrimmed(double newStartTime, double newDuration)
    signal clipSelected()

    x: startTime * pixelsPerSecond
    width: Math.max(20, duration * pixelsPerSecond)
    height: parent ? parent.height - 8 : 52
    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

    // Main Clip Body
    Rectangle {
        id: body
        anchors.fill: parent
        radius: 6
        color: root.clipColor
        border.color: root.isSelected ? "#FFFFFF" : root.accentColor
        border.width: root.isSelected ? 2 : 1
        clip: true

        // Header Strip
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 18
            color: Qt.darker(root.clipColor, 1.3)

            Row {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                Text {
                    text: root.clipType === "video" ? "🎬" : (root.clipType === "audio" ? "🔊" : "✨")
                    font.pixelSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: root.clipName + " [" + root.duration.toFixed(2) + "s]"
                    color: "#FFFFFF"
                    font.pixelSize: 10
                    font.bold: true
                    elide: Text.ElideRight
                    width: parent.width - 24
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Procedural Waveform Canvas for Audio Clips
        Canvas {
            id: waveCanvas
            visible: root.clipType === "audio"
            anchors.fill: parent
            anchors.topMargin: 18
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = "#80E676";
                ctx.lineWidth = 1.5;
                ctx.beginPath();

                var midY = height / 2;
                var step = 4;
                for (var x = 0; x < width; x += step) {
                    var amp = Math.sin(x * 0.08) * Math.cos(x * 0.03) * (height * 0.38);
                    ctx.moveTo(x, midY - amp);
                    ctx.lineTo(x, midY + amp);
                }
                ctx.stroke();
            }
        }

        // Procedural Filmstrip Thumbnails for Video Clips
        Row {
            visible: root.clipType === "video"
            anchors.fill: parent
            anchors.topMargin: 20
            spacing: 4
            clip: true

            Repeater {
                model: Math.max(1, Math.floor(root.width / 60))
                Rectangle {
                    width: 56
                    height: parent.height - 4
                    color: Qt.darker(root.clipColor, 1.5)
                    radius: 3
                    border.color: "#334E68"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: (index * (root.duration / Math.max(1, Math.floor(root.width / 60)))).toFixed(1) + "s"
                        color: "#627D98"
                        font.pixelSize: 8
                        font.family: "Monospace"
                    }
                }
            }
        }

        // Bézier Animation Curve Overlay
        Canvas {
            id: curveCanvas
            anchors.fill: parent
            anchors.topMargin: 18
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                ctx.strokeStyle = root.accentColor;
                ctx.lineWidth = 2;
                ctx.beginPath();

                var k1_x = kf1.x + kf1.width / 2;
                var k1_y = kf1.y + kf1.height / 2 - 18;
                var k2_x = kf2.x + kf2.width / 2;
                var k2_y = kf2.y + kf2.height / 2 - 18;

                ctx.moveTo(0, k1_y);
                ctx.lineTo(k1_x, k1_y);

                // Cubic Bézier curve between keyframe 1 and keyframe 2
                var cp1_x = k1_x + (k2_x - k1_x) * 0.42;
                var cp1_y = k1_y;
                var cp2_x = k1_x + (k2_x - k1_x) * 0.58;
                var cp2_y = k2_y;

                ctx.bezierCurveTo(cp1_x, cp1_y, cp2_x, cp2_y, k2_x, k2_y);
                ctx.lineTo(width, k2_y);
                ctx.stroke();
            }
        }

        // Interactive Keyframe Handles
        KeyframeHandle {
            id: kf1
            timeSeconds: 1.0
            value: 0.2
            propertyName: "Opacity"
            handleColor: root.accentColor
            pixelsPerSecond: root.pixelsPerSecond
            trackHeight: root.height - 18
            onKeyframeMoved: curveCanvas.requestPaint()
        }

        KeyframeHandle {
            id: kf2
            timeSeconds: Math.min(root.duration - 0.5, 4.0)
            value: 0.95
            propertyName: "Opacity"
            handleColor: root.accentColor
            pixelsPerSecond: root.pixelsPerSecond
            trackHeight: root.height - 18
            onKeyframeMoved: curveCanvas.requestPaint()
        }

        // Clip Drag Area (Move horizontally)
        MouseArea {
            id: moveArea
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            cursorShape: Qt.SizeAllCursor
            drag.target: root
            drag.axis: Drag.XAxis
            drag.minimumX: 0

            onPressed: {
                root.isSelected = true
                root.clipSelected()
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
            color: leftMouse.containsMouse || leftMouse.drag.active ? "#FFFFFF" : root.accentColor
            opacity: 0.7

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
                        curveCanvas.requestPaint()
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
            color: rightMouse.containsMouse || rightMouse.drag.active ? "#FFFFFF" : root.accentColor
            opacity: 0.7

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
                        curveCanvas.requestPaint()
                    }
                }
            }
        }
    }
}
