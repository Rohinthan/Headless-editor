import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string trackId: "track_v1"
    property string trackName: "Group 1"
    property string trackType: "video" // "video", "audio", "fx", "text"
    property bool isMuted: false
    property bool isSolo: false
    property bool isLocked: false
    property string trackIcon: "▶"
    property color trackAccent: {
        if (trackType === "video") return "#54A0FF"
        if (trackType === "audio") return "#1DD1A1"
        if (trackType === "fx") return "#FF6E6A"
        if (trackType === "text") return "#FECA57"
        return "#48DBFB"
    }
    property double pixelsPerSecond: 100.0

    default property alias clips: clipContainer.children

    height: 60
    color: "#161822"
    border.color: "#212433"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Track Header (Matching video frams.avif left side badges)
        Rectangle {
            Layout.preferredWidth: 90
            Layout.fillHeight: true
            color: "#13141E"
            border.color: "#212433"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 8

                // Colorful Icon Badge (Matching video frams.avif)
                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: Qt.darker(root.trackAccent, 2.2)
                    border.color: root.trackAccent
                    border.width: 1.5

                    Text {
                        anchors.centerIn: parent
                        text: root.trackIcon
                        color: root.trackAccent
                        font.pixelSize: 14
                        font.bold: true
                    }
                }

                // Mute Toggle Icon
                Rectangle {
                    width: 24
                    height: 24
                    radius: 6
                    color: root.isMuted ? "#3D1B1B" : "#1C1F2D"

                    Text {
                        anchors.centerIn: parent
                        text: root.isMuted ? "🔇" : "👁"
                        font.pixelSize: 11
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.isMuted = !root.isMuted
                    }
                }
            }
        }

        // Track Content Lane (Contains TimelineClips)
        Item {
            id: clipContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            // Grid Background Lines
            Repeater {
                model: Math.max(1, Math.floor(clipContainer.width / (root.pixelsPerSecond * 2)))
                Rectangle {
                    x: index * (root.pixelsPerSecond * 2)
                    width: 1
                    height: parent.height
                    color: "#1C1F2B"
                }
            }
        }
    }
}
