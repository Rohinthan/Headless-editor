import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var timelineController: null
    signal projectCreated(string name, string aspect, int width, int height, double fps, string bgColor)
    signal dialogClosed()

    visible: false
    color: "#D90A0B10" // Dark backdrop overlay
    anchors.fill: parent
    z: 999

    // Consume clicks outside modal
    MouseArea {
        anchors.fill: parent
        onClicked: {} // Block background interaction
    }

    // Modal Card (Matching 1.jpeg)
    Rectangle {
        id: card
        width: Math.min(parent.width - 40, 420)
        height: Math.min(parent.height - 40, 620)
        anchors.centerIn: parent
        radius: 18
        color: "#FFFFFF"
        clip: true

        // Smooth entry animation
        scale: root.visible ? 1.0 : 0.85
        opacity: root.visible ? 1.0 : 0.0
        Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
        Behavior on opacity { NumberAnimation { duration: 180 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            // Top Header: Title
            Text {
                text: "Aspect Ratio"
                color: "#00E5FF"
                font.pixelSize: 18
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            // Tab Selector: [PROJECT] | [ELEMENT]
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                radius: 10
                color: "#EFEFF4"

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 10
                        color: projectTabMouse.checked ? "#1E2235" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "PROJECT"
                            color: projectTabMouse.checked ? "#00E5FF" : "#6E7191"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        MouseArea {
                            id: projectTabMouse
                            property bool checked: true
                            anchors.fill: parent
                            onClicked: { checked = true; elementTabMouse.checked = false; }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 10
                        color: elementTabMouse.checked ? "#1E2235" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "ELEMENT"
                            color: elementTabMouse.checked ? "#00E5FF" : "#6E7191"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        MouseArea {
                            id: elementTabMouse
                            property bool checked: false
                            anchors.fill: parent
                            onClicked: { checked = true; projectTabMouse.checked = false; }
                        }
                    }
                }
            }

            // Project Name Field
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: nameInput
                        Layout.fillWidth: true
                        placeholderText: "New Project"
                        text: "New Project"
                        font.pixelSize: 14
                        color: "#1E2235"
                        background: Rectangle {
                            color: "transparent"
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 1.5
                                color: nameInput.activeFocus ? "#00E5FF" : "#D1D5DB"
                            }
                        }
                    }

                    Text {
                        text: "⚙"
                        font.pixelSize: 16
                        color: "#9CA3AF"
                    }
                }
            }

            // Aspect Ratio Selector Cards (16:9, 9:16, 4:5, 1:1, 4:3, 21:9)
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    property string selectedAspect: "16:9"

                    Repeater {
                        model: [
                            { label: "16:9", aspect: "16:9", w: 1920, h: 1080 },
                            { label: "9:16", aspect: "9:16", w: 1080, h: 1920 },
                            { label: "4:5",  aspect: "4:5",  w: 1080, h: 1350 },
                            { label: "1:1",  aspect: "1:1",  w: 1080, h: 1080 },
                            { label: "4:3",  aspect: "4:3",  w: 1440, h: 1080 }
                        ]

                        Rectangle {
                            id: aspectCard
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: 8
                            color: parent.selectedAspect === modelData.aspect ? "#00E676" : "#F3F4F6"
                            border.color: parent.selectedAspect === modelData.aspect ? "#00C853" : "#D1D5DB"
                            border.width: 1

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2
                                Text {
                                    text: modelData.label
                                    color: aspectCard.parent.selectedAspect === modelData.aspect ? "#FFFFFF" : "#374151"
                                    font.pixelSize: 11
                                    font.bold: true
                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    aspectCard.parent.selectedAspect = modelData.aspect
                                    root.targetWidth = modelData.w
                                    root.targetHeight = modelData.h
                                }
                            }
                        }
                    }
                }
            }

            // Dropdowns: Resolution, Frame Rate, Background
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12

                // Resolution
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Resolution"; color: "#4B5563"; font.pixelSize: 13; Layout.preferredWidth: 100 }
                    ComboBox {
                        id: resCombo
                        Layout.fillWidth: true
                        model: ["1080p (FHD)", "4K UHD (2160p)", "720p (HD)", "540p (SD)"]
                        currentIndex: 0
                        background: Rectangle { color: "#F3F4F6"; radius: 8; border.color: "#E5E7EB" }
                    }
                }

                // Frame Rate
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Frame Rate"; color: "#4B5563"; font.pixelSize: 13; Layout.preferredWidth: 100 }
                    ComboBox {
                        id: fpsCombo
                        Layout.fillWidth: true
                        model: ["60 fps", "30 fps", "24 fps", "12 fps"]
                        currentIndex: 0
                        background: Rectangle { color: "#F3F4F6"; radius: 8; border.color: "#E5E7EB" }
                    }
                }

                // Background
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Background"; color: "#4B5563"; font.pixelSize: 13; Layout.preferredWidth: 100 }
                    ComboBox {
                        id: bgCombo
                        Layout.fillWidth: true
                        model: ["Transparent", "Black", "White", "Dark Grey"]
                        currentIndex: 1
                        background: Rectangle { color: "#F3F4F6"; radius: 8; border.color: "#E5E7EB" }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // Action Button: CREATE PROJECT
            Button {
                id: createBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                text: "CREATE PROJECT"

                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: createBtn.hovered ? "#00C853" : "#00E676"
                    radius: 10
                }

                onClicked: {
                    let w = root.targetWidth;
                    let h = root.targetHeight;
                    if (resCombo.currentIndex === 1) { // 4K
                        w *= 2; h *= 2;
                    } else if (resCombo.currentIndex === 2) { // 720p
                        w = Math.round(w * 0.666); h = Math.round(h * 0.666);
                    }

                    let fpsVal = 60.0;
                    if (fpsCombo.currentIndex === 1) fpsVal = 30.0;
                    else if (fpsCombo.currentIndex === 2) fpsVal = 24.0;
                    else if (fpsCombo.currentIndex === 3) fpsVal = 12.0;

                    let bgColors = ["#00000000", "#000000", "#FFFFFF", "#1E1E1E"];
                    let bg = bgColors[bgCombo.currentIndex];

                    if (root.timelineController) {
                        root.timelineController.createProject(
                            nameInput.text,
                            aspectRow.selectedAspect,
                            w,
                            h,
                            fpsVal,
                            bg
                        );
                    }

                    root.projectCreated(nameInput.text, aspectRow.selectedAspect, w, h, fpsVal, bg);
                    root.visible = false;
                }
            }

            // Bottom Close Icon (✕)
            Rectangle {
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                Layout.alignment: Qt.AlignHCenter
                radius: 16
                color: closeMouse.containsMouse ? "#E5E7EB" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    color: "#6B7280"
                    font.pixelSize: 14
                    font.bold: true
                }

                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.visible = false;
                        root.dialogClosed();
                    }
                }
            }
        }
    }

    property int targetWidth: 1920
    property int targetHeight: 1080
}
