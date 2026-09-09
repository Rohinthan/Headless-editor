import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Antigravity.Video 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1440
    height: 900
    minimumWidth: 1024
    minimumHeight: 700
    title: "Headless-Editor — " + timelineCtrl.projectName + " (" + timelineCtrl.aspectRatio + ")"
    color: "#0F1015"

    TimelineController {
        id: timelineCtrl
        fps: 60.0
        duration: viewport.duration > 0 ? viewport.duration : 14.0
        onPositionChanged: {
            if (Math.abs(viewport.position - timelineCtrl.position) > 0.05) {
                viewport.position = timelineCtrl.position
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Select Video / Audio Media File"
        nameFilters: ["Media Files (*.mp4 *.mkv *.mov *.avi *.webm *.mp3 *.wav *.flac)", "All Files (*)"]
        onAccepted: {
            viewport.openFile(fileDialog.selectedFile.toString())
            timelineCtrl.setDuration(viewport.duration)
        }
    }

    // ==================== NEW PROJECT DIALOG (1.jpeg reference) ====================
    NewProjectDialog {
        id: newProjectDialog
        timelineController: timelineCtrl
        onProjectCreated: function(name, aspect, width, height, fps, bgColor) {
            viewport.resetTransformEffects()
        }
    }

    // ==================== EXPORT DIALOG ====================
    ExportDialog {
        id: exportDialog
        timelineController: timelineCtrl
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==================== TOP HEADER / TOOLBAR ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "#16181F"

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#252836"
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 14

                // Branding & Project Name
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 26
                        height: 26
                        radius: 6
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00E5FF" }
                            GradientStop { position: 1.0; color: "#00E676" }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "▲"
                            color: "#FFFFFF"
                            font.bold: true
                            font.pixelSize: 13
                        }
                    }

                    Text {
                        text: "HEADLESS-EDITOR"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.bold: true
                        font.letterSpacing: 1.5
                    }

                    // Project Aspect Tag
                    Rectangle {
                        height: 20
                        Layout.preferredWidth: aspectTagText.contentWidth + 14
                        radius: 4
                        color: "#202538"
                        border.color: "#00E5FF"
                        border.width: 1
                        Text {
                            id: aspectTagText
                            anchors.centerIn: parent
                            text: timelineCtrl.projectName + " [" + timelineCtrl.aspectRatio + "]"
                            color: "#00E5FF"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }

                Rectangle { width: 1; height: 24; color: "#2E3245" }

                // Actions: New Project (+), Open Media (📁), Export (🚀)
                Button {
                    text: "+ New Project"
                    contentItem: Text {
                        text: parent.text
                        color: "#00E5FF"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#1C2E3D" : "#192230"
                        radius: 6
                        border.color: "#00E5FF"
                    }
                    onClicked: newProjectDialog.visible = true
                }

                Button {
                    text: "📁 Open Media"
                    contentItem: Text {
                        text: parent.text
                        color: "#E0E3EB"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#2A2D3D" : "#1F222F"
                        radius: 6
                        border.color: "#35394D"
                    }
                    onClicked: fileDialog.open()
                }

                Button {
                    text: "🚀 Export Composition"
                    contentItem: Text {
                        text: parent.text
                        color: "#FFFFFF"
                        font.pixelSize: 12
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#00C853" : "#00E676"
                        radius: 6
                    }
                    onClicked: exportDialog.visible = true
                }

                Item { Layout.fillWidth: true }

                // PipeWire Audio & HW Status
                Rectangle {
                    height: 28
                    Layout.preferredWidth: hwStatusText.contentWidth + 28
                    radius: 14
                    color: "#0D2E24"
                    border.color: "#00E676"
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle { width: 8; height: 8; radius: 4; color: "#00E676" }
                        Text {
                            id: hwStatusText
                            text: "⚡ PipeWire 48kHz + VA-API HW Decode"
                            color: "#B9F6CA"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
            }
        }

        // ==================== WORKSPACE (3-COLUMN SPLIT) ====================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---------------- LEFT PANEL: PROJECT MEDIA & AUDIO ----------------
            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: "#13141C"

                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: "#202330"
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    Text {
                        text: "PROJECT ASSETS"
                        color: "#7E849E"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1.0
                    }

                    // Media Bin Card
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 78
                        radius: 8
                        color: "#1C1E2A"
                        border.color: "#00E5FF"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Rectangle {
                                width: 50; height: 50; radius: 6; color: "#000000"; border.color: "#2C3042"
                                Text { anchors.centerIn: parent; text: "🎬"; font.pixelSize: 22 }
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Text {
                                    text: viewport.source !== "" ? viewport.source.split('/').pop() : timelineCtrl.projectName
                                    color: "#FFFFFF"
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: timelineCtrl.aspectRatio + " • " + timelineCtrl.fps + " fps"
                                    color: "#00E676"
                                    font.pixelSize: 10
                                    font.family: "Monospace"
                                }
                                Text {
                                    text: timelineCtrl.canvasWidth + " x " + timelineCtrl.canvasHeight
                                    color: "#80DEEA"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    // Master Volume
                    Text {
                        text: "AUDIO MONITOR"
                        color: "#7E849E"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1.0
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Text { text: "Master Vol:"; color: "#E0E3EB"; font.pixelSize: 11; Layout.fillWidth: true }
                            Slider {
                                from: 0.0; to: 1.5; value: timelineCtrl.volume
                                onMoved: timelineCtrl.setVolume(value)
                            }
                            Text { text: Math.round(timelineCtrl.volume * 100) + "%"; color: "#00E676"; font.pixelSize: 10; font.family: "Monospace" }
                        }

                        Button {
                            Layout.fillWidth: true
                            text: timelineCtrl.isMuted ? "🔇 MUTED (Click to Unmute)" : "🔊 MUTE AUDIO"
                            checkable: true
                            checked: timelineCtrl.isMuted
                            onClicked: timelineCtrl.setMuted(checked)
                            contentItem: Text {
                                text: parent.text
                                color: parent.checked ? "#FF5252" : "#A0A5B8"
                                font.pixelSize: 11
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.checked ? "#3E1B1B" : "#1E2130"
                                radius: 4
                                border.color: parent.checked ? "#FF5252" : "#2F354D"
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // ---------------- CENTER PANEL: VIEWPORT MONITOR ----------------
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0B0C10"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Viewport Monitor Area
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        // Canvas Container (Supports 16:9 Landscape or 9:16 Portrait from for editing line .jpeg)
                        Rectangle {
                            anchors.centerIn: parent
                            width: timelineCtrl.aspectRatio === "9:16" ? Math.min(parent.height * (9.0/16.0), parent.width) : Math.min(parent.width, parent.height * (16.0/9.0))
                            height: timelineCtrl.aspectRatio === "9:16" ? Math.min(parent.height, parent.width * (16.0/9.0)) : Math.min(parent.height, parent.width * (9.0/16.0))
                            color: timelineCtrl.backgroundColor
                            border.color: "#2C3044"
                            border.width: 1

                            ViewportItem {
                                id: viewport
                                anchors.fill: parent
                                focus: true
                            }
                        }

                        // Timecode HUD
                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.margins: 14
                            height: 30
                            width: tcRow.width + 20
                            radius: 6
                            color: "#CC0B0C10"
                            border.color: "#26293A"

                            RowLayout {
                                id: tcRow
                                anchors.centerIn: parent
                                spacing: 8
                                Text { text: "SMPTE"; color: "#00E676"; font.pixelSize: 10; font.bold: true }
                                Text {
                                    text: timelineCtrl.timecode
                                    color: "#FFFFFF"
                                    font.pixelSize: 13
                                    font.bold: true
                                    font.family: "Monospace"
                                }
                            }
                        }
                    }
                }
            }

            // ---------------- RIGHT PANEL: INSPECTOR ----------------
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                color: "#13141C"

                Rectangle {
                    anchors.left: parent.left
                    width: 1
                    height: parent.height
                    color: "#202330"
                }

                ScrollView {
                    anchors.fill: parent
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        anchors.margins: 14
                        spacing: 16

                        Text {
                            text: "TRANSFORM CONTROLS"
                            color: "#7E849E"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Text { text: "Scale: " + viewport.scaleFactor.toFixed(2) + "x"; color: "#E0E3EB"; font.pixelSize: 11; Layout.fillWidth: true }
                                Slider { from: 0.2; to: 2.0; value: viewport.scaleFactor; onMoved: viewport.scaleFactor = value }
                            }

                            RowLayout {
                                Text { text: "Rotation: " + viewport.rotationAngle.toFixed(0) + "°"; color: "#E0E3EB"; font.pixelSize: 11; Layout.fillWidth: true }
                                Slider { from: -180.0; to: 180.0; value: viewport.rotationAngle; onMoved: viewport.rotationAngle = value }
                            }

                            RowLayout {
                                Text { text: "Opacity: " + Math.round(viewport.opacityValue * 100) + "%"; color: "#E0E3EB"; font.pixelSize: 11; Layout.fillWidth: true }
                                Slider { from: 0.0; to: 1.0; value: viewport.opacityValue; onMoved: viewport.opacityValue = value }
                            }
                        }

                        Text {
                            text: "COLOR GRADING (GLSL)"
                            color: "#7E849E"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Text { text: "Brightness: " + viewport.brightness.toFixed(2); color: "#E0E3EB"; font.pixelSize: 11; Layout.fillWidth: true }
                                Slider { from: -0.5; to: 0.5; value: viewport.brightness; onMoved: viewport.brightness = value }
                            }

                            RowLayout {
                                Text { text: "Contrast: " + viewport.contrast.toFixed(2); color: "#E0E3EB"; font.pixelSize: 11; Layout.fillWidth: true }
                                Slider { from: 0.2; to: 2.0; value: viewport.contrast; onMoved: viewport.contrast = value }
                            }

                            RowLayout {
                                Text { text: "Saturation: " + viewport.saturation.toFixed(2); color: "#E0E3EB"; font.pixelSize: 11; Layout.fillWidth: true }
                                Slider { from: 0.0; to: 2.5; value: viewport.saturation; onMoved: viewport.saturation = value }
                            }
                        }
                    }
                }
            }
        }

        // ==================== BOTTOM PANEL: MULTI-TRACK TIMELINE ====================
        TimelineView {
            id: timelineView
            Layout.fillWidth: true
            Layout.preferredHeight: 280
            timelineController: timelineCtrl
            onRequestExport: exportDialog.visible = true
            onRequestNewProject: newProjectDialog.visible = true
        }
    }

    // Keyboard Shortcuts
    Shortcut { sequence: "Space"; onActivated: timelineCtrl.togglePlay() }
    Shortcut { sequence: "Left"; onActivated: timelineCtrl.stepFrame(-1) }
    Shortcut { sequence: "Right"; onActivated: timelineCtrl.stepFrame(1) }
    Shortcut { sequence: "Home"; onActivated: timelineCtrl.jumpToStart() }
    Shortcut { sequence: "End"; onActivated: timelineCtrl.jumpToEnd() }
    Shortcut { sequence: "Ctrl+N"; onActivated: newProjectDialog.visible = true }
    Shortcut { sequence: "Ctrl+E"; onActivated: exportDialog.visible = true }
    Shortcut { sequence: "Ctrl+K"; onActivated: timelineCtrl.splitClipAtPlayhead("clip_main") }
}
