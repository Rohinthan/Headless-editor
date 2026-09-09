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

    signal requestExport()
    signal requestNewProject()

    color: "#0F1017"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==================== TOP TOOLBAR & TRANSPORT STRIP (Matching video frams.avif) ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#161824"
            border.color: "#232738"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 14

                // Undo / Redo Group
                RowLayout {
                    spacing: 4

                    Button {
                        text: "↶"
                        font.pixelSize: 15
                        Layout.preferredWidth: 32; Layout.preferredHeight: 30
                        contentItem: Text { text: parent.text; color: "#A0A5B8"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: parent.hovered ? "#24283A" : "transparent"; radius: 6 }
                    }

                    Button {
                        text: "↷"
                        font.pixelSize: 15
                        Layout.preferredWidth: 32; Layout.preferredHeight: 30
                        contentItem: Text { text: parent.text; color: "#A0A5B8"; font.pixelSize: 15; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: parent.hovered ? "#24283A" : "transparent"; radius: 6 }
                    }
                }

                Rectangle { width: 1; height: 22; color: "#2B2F44" }

                // Transport Controls: [|◀] [▶/⏸] [▶|]
                RowLayout {
                    spacing: 8

                    Button {
                        text: "◀"
                        Layout.preferredWidth: 32; Layout.preferredHeight: 30
                        contentItem: Text { text: parent.text; color: "#FFFFFF"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: parent.hovered ? "#282C40" : "#1C2030"; radius: 6 }
                        onClicked: if (root.timelineController) root.timelineController.stepFrame(-1)
                    }

                    Button {
                        id: playBtn
                        text: root.timelineController && root.timelineController.isPlaying ? "⏸" : "▶"
                        Layout.preferredWidth: 40; Layout.preferredHeight: 32
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: playBtn.hovered ? "#00E5FF" : "#1E293B"
                            radius: 8
                            border.color: "#00E5FF"
                            border.width: 1
                        }
                        onClicked: if (root.timelineController) root.timelineController.togglePlay()
                    }

                    Button {
                        text: "▶"
                        Layout.preferredWidth: 32; Layout.preferredHeight: 30
                        contentItem: Text { text: parent.text; color: "#FFFFFF"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { color: parent.hovered ? "#282C40" : "#1C2030"; radius: 6 }
                        onClicked: if (root.timelineController) root.timelineController.stepFrame(1)
                    }
                }

                Rectangle { width: 1; height: 22; color: "#2B2F44" }

                // Quick Tools: Split/Cut (✂), Add Keyframe (+ ⟐), Bookmark (🔖)
                RowLayout {
                    spacing: 6

                    Button {
                        id: cutBtn
                        text: "✂ CUT / SPLIT"
                        Layout.preferredHeight: 30
                        contentItem: Text {
                            text: parent.text
                            color: "#00E5FF"
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: cutBtn.hovered ? "#1B3B48" : "#192434"
                            radius: 6
                            border.color: "#00E5FF"
                            border.width: 1
                        }
                        onClicked: {
                            if (root.timelineController) {
                                root.timelineController.splitClipAtPlayhead("clip_main")
                            }
                        }
                    }

                    Button {
                        text: "+ ⟐ KEYFRAME"
                        Layout.preferredHeight: 30
                        contentItem: Text {
                            text: parent.text
                            color: "#00E676"
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: "#182C24"
                            radius: 6
                            border.color: "#00E676"
                            border.width: 1
                        }
                    }

                    Button {
                        text: "🔖 MARKER"
                        Layout.preferredHeight: 30
                        contentItem: Text {
                            text: parent.text
                            color: "#FFD600"
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: "#2C2818"
                            radius: 6
                            border.color: "#FFD600"
                            border.width: 1
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Zoom Level Controls
                RowLayout {
                    spacing: 6
                    Text { text: "🔍 Zoom:"; color: "#8C92A4"; font.pixelSize: 11 }
                    Slider {
                        from: 20.0; to: 250.0; value: root.pixelsPerSecond
                        onMoved: root.pixelsPerSecond = value
                    }
                }

                // Export Button (Matching video frams.avif top right)
                Button {
                    id: exportTopBtn
                    text: "⤓ EXPORT"
                    Layout.preferredHeight: 30
                    contentItem: Text {
                        text: parent.text
                        color: "#FFFFFF"
                        font.pixelSize: 11
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: exportTopBtn.hovered ? "#00C853" : "#00E676"
                        radius: 6
                    }
                    onClicked: root.requestExport()
                }
            }
        }

        // ==================== TIME RULER & SCROLL AREA ====================
        ScrollView {
            id: timelineScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: Math.max(width, (root.totalDuration + 5.0) * root.pixelsPerSecond + 100)
            clip: true

            Item {
                width: timelineScrollView.contentWidth
                height: trackColumn.height + 40

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
                        x: 0; y: 0; width: 90; height: parent.height
                        color: "#13141E"
                        border.color: "#26293C"
                        Text {
                            anchors.centerIn: parent
                            text: "LAYERS"
                            color: "#7E849E"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    // Ticks & Numbers
                    Item {
                        x: 90
                        y: 0
                        width: parent.width - 90
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
                                if (root.timelineController) root.timelineController.seek(sec);
                            }
                            onPositionChanged: function(mouse) {
                                if (pressed) {
                                    var sec = Math.max(0.0, mouse.x / root.pixelsPerSecond);
                                    if (root.timelineController) root.timelineController.seek(sec);
                                }
                            }
                        }
                    }
                }

                // ---------------- TRACKS CONTAINER (Matching video frams.avif layers) ----------------
                Column {
                    id: trackColumn
                    x: 0
                    y: 32
                    width: parent.width
                    spacing: 4

                    // Layer 1: Group 1 (Coral Red)
                    TimelineTrack {
                        trackId: "t_group1"
                        trackName: "Group 1"
                        trackType: "fx"
                        trackIcon: "↗"
                        trackAccent: "#FF6E6A"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_group1"
                            clipName: "Group 1"
                            clipType: "fx"
                            startTime: 1.0
                            duration: 12.0
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Layer 2: Plus 2 (Teal)
                    TimelineTrack {
                        trackId: "t_plus2"
                        trackName: "Plus 2"
                        trackType: "fx"
                        trackIcon: "✚"
                        trackAccent: "#48DBFB"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_plus2"
                            clipName: "Plus 2"
                            clipType: "fx"
                            startTime: 0.0
                            duration: 14.0
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Layer 3: Multifoil 1 (Mint Green)
                    TimelineTrack {
                        trackId: "t_multi1"
                        trackName: "Multifoil 1"
                        trackType: "video"
                        trackIcon: "✱"
                        trackAccent: "#1DD1A1"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_multi1"
                            clipName: "Multifoil 1"
                            clipType: "video"
                            startTime: 2.5
                            duration: 10.5
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Layer 4: Plus 1 (Sky Blue)
                    TimelineTrack {
                        trackId: "t_plus1"
                        trackName: "Plus 1"
                        trackType: "video"
                        trackIcon: "✚"
                        trackAccent: "#54A0FF"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_main"
                            clipName: "Plus 1"
                            clipType: "video"
                            startTime: 1.5
                            duration: 11.5
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Layer 5: Polygon 1 (Teal)
                    TimelineTrack {
                        trackId: "t_poly1"
                        trackName: "Polygon 1"
                        trackType: "fx"
                        trackIcon: "⬡"
                        trackAccent: "#48DBFB"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_poly1"
                            clipName: "Polygon 1"
                            clipType: "fx"
                            startTime: 0.0
                            duration: 14.0
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }

                    // Layer 6: Text Layer (Gold/Yellow)
                    TimelineTrack {
                        trackId: "t_text1"
                        trackName: "Text Layer"
                        trackType: "text"
                        trackIcon: "T"
                        trackAccent: "#FECA57"
                        pixelsPerSecond: root.pixelsPerSecond
                        width: parent.width

                        TimelineClip {
                            clipId: "clip_text1"
                            clipName: "Text Layer"
                            clipType: "text"
                            startTime: 0.0
                            duration: 14.0
                            pixelsPerSecond: root.pixelsPerSecond
                        }
                    }
                }

                // ==================== WHITE EDITING LINE & FLOATING TIMECODE (Matching video frams.avif) ====================
                Item {
                    id: editingLineContainer
                    x: 90 + (root.playheadPosition * root.pixelsPerSecond)
                    y: 0
                    width: 60
                    height: parent.height
                    z: 200

                    // Floating Timecode Badge (Centered exactly over the white line)
                    Rectangle {
                        anchors.horizontalCenter: parent.left
                        anchors.top: parent.top
                        anchors.topMargin: 2
                        width: tcLabel.contentWidth + 16
                        height: 22
                        radius: 6
                        color: "#181A24"
                        border.color: "#4A5568"
                        border.width: 1

                        Text {
                            id: tcLabel
                            anchors.centerIn: parent
                            text: root.timelineController ? root.timelineController.timecode : "00:00:00"
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                            font.family: "Monospace"
                        }
                    }

                    // White Editing Line Needle (Full Height across all tracks)
                    Rectangle {
                        anchors.horizontalCenter: parent.left
                        anchors.top: parent.top
                        anchors.topMargin: 26
                        anchors.bottom: parent.bottom
                        width: 2
                        color: "#FFFFFF"

                        // Subtle outer glow
                        Rectangle {
                            anchors.centerIn: parent
                            width: 6
                            height: parent.height
                            color: "#40FFFFFF"
                            radius: 3
                            z: -1
                        }
                    }

                    // Drag scrubber mouse area
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.SplitHCursor
                        drag.target: editingLineContainer
                        drag.axis: Drag.XAxis
                        drag.minimumX: 90

                        onPositionChanged: {
                            if (drag.active) {
                                var sec = Math.max(0.0, (editingLineContainer.x - 90) / root.pixelsPerSecond);
                                if (root.timelineController) root.timelineController.seek(sec);
                            }
                        }
                    }
                }
            }
        }
    }
}
