import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root

    property var timelineController: null
    visible: false
    color: "#D90A0B10" // Dark backdrop overlay
    anchors.fill: parent
    z: 999

    MouseArea {
        anchors.fill: parent
        onClicked: {} // Block background clicks
    }

    Rectangle {
        id: card
        width: Math.min(parent.width - 40, 520)
        height: Math.min(parent.height - 40, 540)
        anchors.centerIn: parent
        radius: 16
        color: "#181A24"
        border.color: "#2C3042"
        border.width: 1
        clip: true

        scale: root.visible ? 1.0 : 0.85
        opacity: root.visible ? 1.0 : 0.0
        Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
        Behavior on opacity { NumberAnimation { duration: 180 } }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 16

            // Header Strip
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "🎬 Export Video Composition"
                    color: "#FFFFFF"
                    font.pixelSize: 17
                    font.bold: true
                    Layout.fillWidth: true
                }

                Rectangle {
                    width: 28; height: 28; radius: 14; color: "#232738"
                    Text { anchors.centerIn: parent; text: "✕"; color: "#A0A5B8"; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.visible = false
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#2A2E40" }

            // Export Settings Form
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12

                // File Name
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "File Name:"; color: "#9EA3B8"; font.pixelSize: 12; Layout.preferredWidth: 100 }
                    TextField {
                        id: filenameField
                        Layout.fillWidth: true
                        text: (root.timelineController ? root.timelineController.projectName.replace(' ', '_') : "My_Render") + ".mp4"
                        color: "#FFFFFF"
                        background: Rectangle { color: "#1F2333"; radius: 6; border.color: "#353A52" }
                    }
                }

                // Format
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Format:"; color: "#9EA3B8"; font.pixelSize: 12; Layout.preferredWidth: 100 }
                    ComboBox {
                        id: formatCombo
                        Layout.fillWidth: true
                        model: ["MP4 (H.264 / AAC - High Compatibility)", "Apple ProRes 422 (Broadcast Master)", "MKV (Lossless High Speed)"]
                        currentIndex: 0
                        background: Rectangle { color: "#1F2333"; radius: 6; border.color: "#353A52" }
                    }
                }

                // Resolution
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Resolution:"; color: "#9EA3B8"; font.pixelSize: 12; Layout.preferredWidth: 100 }
                    ComboBox {
                        id: resCombo
                        Layout.fillWidth: true
                        model: [
                            "Match Project (" + (root.timelineController ? root.timelineController.canvasWidth + "x" + root.timelineController.canvasHeight : "1920x1080") + ")",
                            "1080p FHD (1920 x 1080)",
                            "4K UHD (3840 x 2160)",
                            "720p HD (1280 x 720)",
                            "9:16 Portrait / Reels (1080 x 1920)"
                        ]
                        currentIndex: 0
                        background: Rectangle { color: "#1F2333"; radius: 6; border.color: "#353A52" }
                    }
                }

                // Frame Rate
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Frame Rate:"; color: "#9EA3B8"; font.pixelSize: 12; Layout.preferredWidth: 100 }
                    ComboBox {
                        id: fpsCombo
                        Layout.fillWidth: true
                        model: ["60 fps (Smooth Motion)", "30 fps (Standard Web/TV)", "24 fps (Cinematic Film)"]
                        currentIndex: 0
                        background: Rectangle { color: "#1F2333"; radius: 6; border.color: "#353A52" }
                    }
                }

                // Quality / Bitrate
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Bitrate:"; color: "#9EA3B8"; font.pixelSize: 12; Layout.preferredWidth: 100 }
                    ComboBox {
                        id: bitrateCombo
                        Layout.fillWidth: true
                        model: ["High (16,000 kbps - Studio Master)", "Standard (8,000 kbps - YouTube/Vimeo)", "Low (4,000 kbps - Quick Preview)"]
                        currentIndex: 1
                        background: Rectangle { color: "#1F2333"; radius: 6; border.color: "#353A52" }
                    }
                }
            }

            // Export Progress Area
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.timelineController ? root.timelineController.isExporting || root.timelineController.exportProgress > 0 : false
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: root.timelineController ? root.timelineController.exportStatus : "Ready"
                        color: "#00E5FF"
                        font.pixelSize: 11
                        font.family: "Monospace"
                        Layout.fillWidth: true
                    }
                    Text {
                        text: (root.timelineController ? root.timelineController.exportProgress.toFixed(1) : "0.0") + "%"
                        color: "#00E676"
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "Monospace"
                    }
                }

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0.0
                    to: 100.0
                    value: root.timelineController ? root.timelineController.exportProgress : 0.0
                    background: Rectangle { color: "#232738"; radius: 4; height: 8 }
                    contentItem: Item {
                        Rectangle {
                            width: parent.width * (parent.parent.visualPosition)
                            height: 8
                            radius: 4
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: "#00E5FF" }
                                GradientStop { position: 1.0; color: "#00E676" }
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            // Action Buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    text: "Close"
                    contentItem: Text {
                        text: parent.text
                        color: "#A0A5B8"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle { color: "#232738"; radius: 8 }
                    onClicked: {
                        if (root.timelineController && root.timelineController.isExporting) {
                            root.timelineController.cancelExport()
                        }
                        root.visible = false
                    }
                }

                Button {
                    id: exportActionBtn
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    text: root.timelineController && root.timelineController.isExporting ? "⏹ CANCEL EXPORT" : "🚀 START EXPORT"
                    contentItem: Text {
                        text: parent.text
                        color: "#FFFFFF"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: root.timelineController && root.timelineController.isExporting ? "#E53935" : (exportActionBtn.hovered ? "#00C853" : "#00E676")
                        radius: 8
                    }

                    onClicked: {
                        if (!root.timelineController) return;

                        if (root.timelineController.isExporting) {
                            root.timelineController.cancelExport()
                            return;
                        }

                        let w = root.timelineController.canvasWidth;
                        let h = root.timelineController.canvasHeight;
                        if (resCombo.currentIndex === 1) { w = 1920; h = 1080; }
                        else if (resCombo.currentIndex === 2) { w = 3840; h = 2160; }
                        else if (resCombo.currentIndex === 3) { w = 1280; h = 720; }
                        else if (resCombo.currentIndex === 4) { w = 1080; h = 1920; }

                        let fpsVal = 60.0;
                        if (fpsCombo.currentIndex === 1) fpsVal = 30.0;
                        else if (fpsCombo.currentIndex === 2) fpsVal = 24.0;

                        let bitrate = 8000;
                        if (bitrateCombo.currentIndex === 0) bitrate = 16000;
                        else if (bitrateCombo.currentIndex === 2) bitrate = 4000;

                        let fmt = formatCombo.currentIndex === 1 ? "prores" : "mp4";
                        let outPath = "/home/raccoon/Editor/" + filenameField.text;

                        root.timelineController.startExport(outPath, fmt, w, h, fpsVal, bitrate);
                    }
                }
            }
        }
    }
}
