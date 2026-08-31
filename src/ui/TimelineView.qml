import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var timelineController: null
    property double pixelsPerSecond: 80.0
    property double totalDuration: timelineController ? timelineController.duration : 30.0
    property double playheadPosition: timelineController ? timelineController.position : 0.0
    property bool snappingEnabled: true

    color: "#0F1017"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==================== TIMELINE TOOLBAR ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: "#161822"
            border.color: "#212433"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                Text {
                    text: "TIMELINE TRACKS"
                    color: "#7E849E"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1.0
                }

                // Snap Toggle Button
                Button {
                    text: "🧲 SNAP"
                    checkable: true
                    checked: root.snappingEnabled
                    onClicked: root.snappingEnabled = checked
                    contentItem: Text {
                        text: parent.text
                        color: parent.checked ? "#00E5FF" : "#9E9E9E"
                        font.pixelSize: 10
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.checked ? "#1E2C3D" : "#1F222E"
                        radius: 4
                        border.color: parent.checked ? "#00E5FF" : "#2E3245"
                    }
                }

                // In / Out Points HUD
                Rectangle {
                    height: 24
                    Layout.preferredWidth: 160
                    radius: 4
                    color: "#191B26"
                    border.color: "#282B3C"

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        Text {
                            text: "IN: " + (root.timelineController ? root.timelineController.inPoint.toFixed(1) + "s" : "0.0s")
                            color: "#00E676"
                            font.pixelSize: 10
                            font.family: "Monospace"
                        }
                        Text {
                            text: "OUT: " + (root.timelineController ? root.timelineController.outPoint.toFixed(1) + "s" : "30.0s")
                            color: "#FF5252"
                            font.pixelSize: 10
                            font.family: "Monospace"
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Zoom Level Controls
                RowLayout {
                    spacing: 6
                    Text { text: "Zoom:"; color: "#8C92A4"; font.pixelSize: 11 }
                    Slider {
                        id: zoomSlider
                        from: 20.0
                        to: 250.0
                        value: root.pixelsPerSecond
                        onMoved: root.pixelsPerSecond = value
                    }
                    Text {
                        text: Math.round(root.pixelsPerSecond) + " px/s"
                        color: "#E2E5F0"
                        font.pixelSize: 10
                        font.family: "Monospace"
                    }
                }
            }
        }

        // ==================== TIME RULER & SCROLL AREA ====================
        ScrollView {
            id: timelineScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: Math.max(width, (root.totalDuration + 5.0) * root.pixelsPerSecond + 140)
            clip: true

            Item {
                width: timelineScrollView.contentWidth
                height: trackColumn.height + 36

                // ---------------- RULER HEADER ----------------
                Rectangle {
                    id: rulerHeader
                    x: 0
                    y: 0
                    width: parent.width
                    height: 32
                    color: "#181A26"
                    border.color: "#26293C"
                    border.width: 1
                    z: 50

                    // Left Header Spacer
                    Rectangle {
                        x: 0; y: 0; width: 140; height: parent.height
                        color: "#141620"
                        border.color: "#26293C"
                        Text {
                            anchors.centerIn: parent
                            text: "TIMECODE"
                            color: "#7E849E"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    // Ticks & Numbers
                    Item {
                        x: 140
                        y: 0
                        width: parent.width - 140
                        height: parent.height

                        Repeater {
                            model: Math.ceil(root.totalDuration + 5)
                            Item {
                                x: index * root.pixelsPerSecond
                                width: root.pixelsPerSecond
                                height: 32

                                // Major tick mark (every 1 sec)
                                Rectangle {
                                    x: 0; y: 16; width: 1; height: 16; color: "#545B7A"
                                }

                                // Sub-ticks (every 0.25 sec)
                                Repeater {
                                    model: 3
                                    Rectangle {
                                        x: (index + 1) * (root.pixelsPerSecond / 4)
                                        y: 22; width: 1; height: 10; color: "#32374E"
                                    }
                                }

                                Text {
                                    x: 4; y: 2
                                    text: (index < 10 ? "00:0" : "00:") + index + ":00"
                                    color: "#9EA3B8"
                                    font.pixelSize: 9
                                    font.family: "Monospace"
                                }
                            }
                        }

                        // Scrubber Click MouseArea
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onPressed: function(mouse) {
                                var sec = Math.max(0.0, mouse.x / root.pixelsPerSecond);
                                if (root.timelineController) {
                                    root.timelineController.seek(sec);
                                }
                            }
                            onPositionChanged: function(mouse) {
                                if (pressed) {
                                    var sec = Math.max(0.0, mouse.x / root.pixelsPerSecond);
                                    if (root.timelineController) {
                                        root.timelineController.seek(sec);
                                    }
                                }
                            }
                        }
                    }
                }

                // ---------------- TRACKS CONTAINER ----------------
                Column {
                    id: trackColumn
                    x: 0
                    y: 32
                    width: parent.width
                    spacing: 4

                    // Track V2: Overlay & FX
                    TimelineTrack {
                        trackId: "v2"
                        trackName: "V2 Overlay"
                        trackType: "fx"
                        trackAccent: "#E040FB"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_pip"
                            clipName: "PiP Overlay Card"
                            clipType: "fx"
                            startTime: 3.0
                            duration: 8.0
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Track V1: Main Video
                    TimelineTrack {
                        trackId: "v1"
                        trackName: "V1 Video"
                        trackType: "video"
                        trackAccent: "#00E5FF"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_main"
                            clipName: "Master Video Stream"
                            clipType: "video"
                            startTime: 0.0
                            duration: root.totalDuration
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Track A1: Primary Audio (PipeWire 48kHz Stereo)
                    TimelineTrack {
                        trackId: "a1"
                        trackName: "A1 Dialog"
                        trackType: "audio"
                        trackAccent: "#00E676"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_audio1"
                            clipName: "Dialogue_Master_48k.wav"
                            clipType: "audio"
                            startTime: 0.0
                            duration: root.totalDuration
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Track A2: BGM / Music
                    TimelineTrack {
                        trackId: "a2"
                        trackName: "A2 Music"
                        trackType: "audio"
                        trackAccent: "#FFB300"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_audio2"
                            clipName: "Cinematic_Score_Stereo.flac"
                            clipType: "audio"
                            startTime: 2.0
                            duration: Math.max(5.0, root.totalDuration - 2.0)
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }
                }

                // ---------------- SYNCHRONIZED PLAYHEAD ----------------
                Item {
                    id: playhead
                    x: 140 + (root.playheadPosition * root.pixelsPerSecond) - 8
                    y: 0
                    width: 16
                    height: parent.height
                    z: 100

                    // Red Cursor Head
                    Canvas {
                        width: 16
                        height: 16
                        y: 8
                        onPaint: {
                            var ctx = getContext("2d");
                            ctx.clearRect(0, 0, width, height);
                            ctx.fillStyle = "#FF1744";
                            ctx.beginPath();
                            ctx.moveTo(0, 0);
                            ctx.lineTo(16, 0);
                            ctx.lineTo(16, 8);
                            ctx.lineTo(8, 16);
                            ctx.lineTo(0, 8);
                            ctx.closePath();
                            ctx.fill();
                        }
                    }

                    // Playhead Needle Line
                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.topMargin: 24
                        anchors.bottom: parent.bottom
                        width: 2
                        color: "#FF1744"
                    }

                    // Playhead Dragger
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SplitHCursor
                        drag.target: playhead
                        drag.axis: Drag.XAxis
                        drag.minimumX: 132

                        onPositionChanged: {
                            if (drag.active) {
                                var sec = Math.max(0.0, (playhead.x + 8 - 140) / root.pixelsPerSecond);
                                if (root.timelineController) {
                                    root.timelineController.seek(sec);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
