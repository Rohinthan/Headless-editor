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
    title: "Headless-Editor — Linux Native Video & Audio NLE Core"
    color: "#0F1015"

    TimelineController {
        id: timelineCtrl
        fps: 30.0
        duration: viewport.duration > 0 ? viewport.duration : 30.0
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

                // Branding
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 24
                        height: 24
                        radius: 6
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00E5FF" }
                            GradientStop { position: 1.0; color: "#7C4DFF" }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "▲"
                            color: "#FFFFFF"
                            font.bold: true
                            font.pixelSize: 12
                        }
                    }

                    Text {
                        text: "HEADLESS-EDITOR"
                        color: "#FFFFFF"
                        font.pixelSize: 15
                        font.bold: true
                        font.letterSpacing: 1.5
                    }

                    Rectangle {
                        width: 48
                        height: 18
                        radius: 4
                        color: "#2A2D3D"
                        Text {
                            anchors.centerIn: parent
                            text: "AUDIO+"
                            color: "#00E676"
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }
                }

                Rectangle { width: 1; height: 24; color: "#2E3245" }

                // Actions
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
                    text: "⟲ Reset FX"
                    contentItem: Text {
                        text: parent.text
                        color: "#A0A5B8"
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? "#2A2D3D" : "transparent"
                        radius: 6
                    }
                    onClicked: viewport.resetTransformEffects()
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
                        Rectangle {
                            width: 8; height: 8; radius: 4; color: "#00E676"
                        }
                        Text {
                            id: hwStatusText
                            text: "⚡ PipeWire 48kHz (Low-Latency) + VA-API Active"
                            color: "#B9F6CA"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }

                // Audio Latency & Drift Telemetry
                Rectangle {
                    height: 28
                    Layout.preferredWidth: 170
                    radius: 6
                    color: "#1B1E2B"
                    border.color: "#2E3245"
                    Text {
                        anchors.centerIn: parent
                        text: "Audio Latency: " + timelineCtrl.audioLatencyMs.toFixed(1) + "ms | A/V: ±0.0ms"
                        color: "#9EA3B8"
                        font.pixelSize: 10
                        font.family: "Monospace"
                    }
                }
            }
        }

        // ==================== WORKSPACE (3-COLUMN SPLIT) ====================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---------------- LEFT PANEL ----------------
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
                        text: "AUDIO & MEDIA POOL"
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
                                    text: viewport.source !== "" ? viewport.source.split('/').pop() : "Synthetic 48kHz Source"
                                    color: "#FFFFFF"
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: "Stereo Float • 48000 Hz"
                                    color: "#00E676"
                                    font.pixelSize: 10
                                    font.family: "Monospace"
                                }
                                Text {
                                    text: "Master Clock: Active"
                                    color: "#80DEEA"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    // Volume & Master Audio Controls
                    Text {
                        text: "MASTER AUDIO CONTROLS"
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

                    // Viewport Monitor
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ViewportItem {
                            id: viewport
                            anchors.fill: parent
                            focus: true
                        }

                        // Timecode HUD
                        Rectangle {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.margins: 14
                            height: 32
                            width: tcRow.width + 20
                            radius: 6
                            color: "#CC0B0C10"
                            border.color: "#26293A"

                            RowLayout {
                                id: tcRow
                                anchors.centerIn: parent
                                spacing: 8
                                Text { text: "MASTER"; color: "#00E676"; font.pixelSize: 10; font.bold: true }
                                Text {
                                    text: timelineCtrl.timecode
                                    color: "#00E5FF"
                                    font.pixelSize: 14
                                    font.bold: true
                                    font.family: "Monospace"
                                }
                            }
                        }
                    }

                    // Transport Control Bar
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        color: "#14161F"
                        border.color: "#232636"
                        border.width: 1

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 12

                            Button {
                                text: "⏮"
                                font.pixelSize: 14
                                onClicked: timelineCtrl.jumpToStart()
                            }

                            Button {
                                text: "◀ -1F"
                                font.pixelSize: 11
                                onClicked: timelineCtrl.stepFrame(-1)
                            }

                            Button {
                                text: timelineCtrl.isPlaying ? "⏸ PAUSE" : "▶ PLAY"
                                font.pixelSize: 13
                                font.bold: true
                                contentItem: Text {
                                    text: parent.text
                                    color: timelineCtrl.isPlaying ? "#FFAB00" : "#00E5FF"
                                    font.pixelSize: 13
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: "#1F2333"
                                    radius: 6
                                    border.color: timelineCtrl.isPlaying ? "#FFAB00" : "#00E5FF"
                                }
                                onClicked: timelineCtrl.togglePlay()
                            }

                            Button {
                                text: "+1F ▶"
                                font.pixelSize: 11
                                onClicked: timelineCtrl.stepFrame(1)
                            }

                            Button {
                                text: "⏭"
                                font.pixelSize: 14
                                onClicked: timelineCtrl.jumpToEnd()
                            }
                        }
                    }
                }
            }

            // ---------------- RIGHT PANEL: INSPECTOR ----------------
            Rectangle {
                Layout.preferredWidth: 300
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
                            text: "GPU & TRANSFORM INSPECTOR"
                            color: "#7E849E"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        // Transform Sliders
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
            Layout.preferredHeight: 240
            timelineController: timelineCtrl
        }
    }

    // Keyboard Shortcuts
    Shortcut { sequence: "Space"; onActivated: timelineCtrl.togglePlay() }
    Shortcut { sequence: "Left"; onActivated: timelineCtrl.stepFrame(-1) }
    Shortcut { sequence: "Right"; onActivated: timelineCtrl.stepFrame(1) }
    Shortcut { sequence: "Home"; onActivated: timelineCtrl.jumpToStart() }
    Shortcut { sequence: "End"; onActivated: timelineCtrl.jumpToEnd() }
}
