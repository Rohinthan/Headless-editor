import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string trackId: "track_v1"
    property string trackName: "V1 Video"
    property string trackType: "video" // "video", "audio", "fx"
    property bool isMuted: false
    property bool isSolo: false
    property bool isLocked: false
    property color trackAccent: trackType === "video" ? "#00E5FF" : (trackType === "audio" ? "#00E676" : "#E040FB")
    property double pixelsPerSecond: 100.0

    default property alias clips: clipContainer.children

    height: 64
    color: "#151720"
    border.color: "#212433"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Track Header Control Area
        Rectangle {
            Layout.preferredWidth: 140
            Layout.fillHeight: true
            color: "#191B26"
            border.color: "#242838"
            border.width: 1

            // Track Color Tag
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 4
                color: root.trackAccent
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                spacing: 4

                // Title & Type Badge
                RowLayout {
                    spacing: 6
                    Rectangle {
                        width: 24
                        height: 18
                        radius: 3
                        color: Qt.darker(root.trackAccent, 2.0)
                        border.color: root.trackAccent
                        Text {
                            anchors.centerIn: parent
                            text: root.trackName.split(' ')[0]
                            color: "#FFFFFF"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    Text {
                        text: root.trackName
                        color: "#E2E5F0"
                        font.pixelSize: 11
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                // Buttons: Mute, Solo, Lock
                RowLayout {
                    spacing: 4

                    Button {
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 22
                        checkable: true
                        checked: root.isMuted
                        onClicked: root.isMuted = checked
                        contentItem: Text {
                            text: "M"
                            color: parent.checked ? "#FF5252" : "#9E9E9E"
                            font.pixelSize: 10
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.checked ? "#3E1B1B" : "#242838"
                            radius: 3
                        }
                    }

                    Button {
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 22
                        checkable: true
                        checked: root.isSolo
                        onClicked: root.isSolo = checked
                        contentItem: Text {
                            text: "S"
                            color: parent.checked ? "#FFD600" : "#9E9E9E"
                            font.pixelSize: 10
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.checked ? "#3E381B" : "#242838"
                            radius: 3
                        }
                    }

                    Button {
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 22
                        checkable: true
                        checked: root.isLocked
                        onClicked: root.isLocked = checked
                        contentItem: Text {
                            text: "🔒"
                            font.pixelSize: 9
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.checked ? "#2D3748" : "#242838"
                            radius: 3
                        }
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

            // Track background grid lines
            Repeater {
                model: Math.max(1, Math.floor(clipContainer.width / (root.pixelsPerSecond * 5)))
                Rectangle {
                    x: index * (root.pixelsPerSecond * 5)
                    width: 1
                    height: parent.height
                    color: "#1E2130"
                }
            }
        }
    }
}
