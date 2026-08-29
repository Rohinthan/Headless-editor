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
    title: "Antigravity NLE Core — Linux High-Performance Video Engine"
    color: "#0F1015"

    // Helper functions for timecode formatting
    function formatTimecode(seconds, fps) {
        if (isNaN(seconds) || seconds < 0) seconds = 0;
        if (isNaN(fps) || fps <= 0) fps = 30;

        let totalFrames = Math.floor(seconds * fps);
        let frames = totalFrames % Math.round(fps);
        let totalSecs = Math.floor(seconds);
        let s = totalSecs % 60;
        let m = Math.floor(totalSecs / 60) % 60;
        let h = Math.floor(totalSecs / 3600);

        let pad = function(n) { return (n < 10 ? "0" : "") + n; };
        return pad(h) + ":" + pad(m) + ":" + pad(s) + ":" + pad(frames);
    }

    FileDialog {
        id: fileDialog
        title: "Select Video Media File"
        nameFilters: ["Video Files (*.mp4 *.mkv *.mov *.avi *.webm *.ts)", "All Files (*)"]
        onAccepted: {
            viewport.openFile(fileDialog.selectedFile.toString())
        }
    }

    // Main Column Layout
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

                // Brand
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
                        text: "ANTIGRAVITY"
                        color: "#FFFFFF"
                        font.pixelSize: 15
                        font.bold: true
                        font.letterSpacing: 1.5
                    }

                    Rectangle {
                        width: 42
                        height: 18
                        radius: 4
                        color: "#2A2D3D"
                        Text {
                            anchors.centerIn: parent
                            text: "PRO"
                            color: "#00E5FF"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }

                Rectangle { width: 1; height: 24; color: "#2E3245" }

                // File Operations
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

                // Hardware Acceleration Badge
                Rectangle {
                    height: 28
                    Layout.preferredWidth: hwStatusText.contentWidth + 32
                    radius: 14
                    color: viewport.hwAccelStatus.indexOf("Hardware") !== -1 ? "#0D2E24" : "#2E2412"
                    border.color: viewport.hwAccelStatus.indexOf("Hardware") !== -1 ? "#00E676" : "#FFB300"
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: viewport.hwAccelStatus.indexOf("Hardware") !== -1 ? "#00E676" : "#FFB300"
                        }
                        Text {
                            id: hwStatusText
                            text: viewport.hwAccelStatus
                            color: viewport.hwAccelStatus.indexOf("Hardware") !== -1 ? "#B9F6CA" : "#FFE082"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }

                // Resolution & FPS Status
                Rectangle {
                    height: 28
                    Layout.preferredWidth: 150
                    radius: 6
                    color: "#1B1E2B"
                    border.color: "#2E3245"
                    Text {
                        anchors.centerIn: parent
                        text: "1920x1080 • " + viewport.fps.toFixed(1) + " FPS"
                        color: "#9EA3B8"
                        font.pixelSize: 11
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

            // ---------------- LEFT PANEL: MEDIA POOL & KEYFRAMES ----------------
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
                        text: "PROJECT MEDIA & NODES"
                        color: "#7E849E"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1.0
                    }

                    // Media Bin Item
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 74
                        radius: 8
                        color: "#1C1E2A"
                        border.color: "#00E5FF"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Rectangle {
                                width: 54
                                height: 54
                                radius: 6
                                color: "#000000"
                                border.color: "#2C3042"
                                Text {
                                    anchors.centerIn: parent
                                    text: "🎬"
                                    font.pixelSize: 22
                                }
                            }

                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Text {
                                    text: viewport.source !== "" ? viewport.source.split('/').pop() : "Test Generator Card"
                                    color: "#FFFFFF"
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: viewport.duration.toFixed(2) + "s • H.264 / VAAPI"
                                    color: "#7E849E"
                                    font.pixelSize: 11
                                }
                                Text {
                                    text: "Status: Active Stream"
                                    color: "#00E676"
                                    font.pixelSize: 10
                                }
                            }
                        }
                    }

                    // Bézier Easing Presets
                    Text {
                        text: "BÉZIER EASING CURVES"
                        color: "#7E849E"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1.0
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: ListModel {
                            ListElement { name: "Cubic Ease-In-Out"; formula: "P(t): (0.42, 0, 0.58, 1)"; icon: "〰" }
                            ListElement { name: "Cubic Ease-In"; formula: "P(t): (0.42, 0, 1.00, 1)"; icon: "◜" }
                            ListElement { name: "Cubic Ease-Out"; formula: "P(t): (0.00, 0, 0.58, 1)"; icon: "◝" }
                            ListElement { name: "Linear Interpolation"; formula: "P(t): (0.00, 0, 1.00, 1)"; icon: "╱" }
                            ListElement { name: "Custom Newton Bézier"; formula: "Newton-Raphson Solver"; icon: "∿" }
                        }
                        delegate: Rectangle {
                            width: parent.width
                            height: 48
                            radius: 6
                            color: index === 0 ? "#222636" : "#171922"
                            border.color: index === 0 ? "#7C4DFF" : "#242838"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 8
                                Text {
                                    text: model.icon
                                    color: "#00E5FF"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                                ColumnLayout {
                                    spacing: 1
                                    Text {
                                        text: model.name
                                        color: "#E2E5F0"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                    Text {
                                        text: model.formula
                                        color: "#7E849E"
                                        font.pixelSize: 9
                                        font.family: "Monospace"
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ---------------- CENTER PANEL: VIEWPORT & MONITOR ----------------
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

                        ViewportItem {
                            id: viewport
                            anchors.fill: parent
                            focus: true
                        }

                        // Timecode & HUD Overlay
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
                                Text {
                                    text: "REC"
                                    color: "#FF5252"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                                Text {
                                    text: root.formatTimecode(viewport.position, viewport.fps)
                                    color: "#00E5FF"
                                    font.pixelSize: 14
                                    font.bold: true
                                    font.family: "Monospace"
                                }
                                Text {
                                    text: "/ " + root.formatTimecode(viewport.duration, viewport.fps)
                                    color: "#7E849E"
                                    font.pixelSize: 12
                                    font.family: "Monospace"
                                }
                            }
                        }
                    }

                    // Transport Control Bar
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        color: "#14161F"

                        Rectangle {
                            anchors.top: parent.top
                            width: parent.width
                            height: 1
                            color: "#232636"
                        }

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 12

                            Button {
                                text: "⏮"
                                font.pixelSize: 14
                                onClicked: viewport.seek(0.0)
                            }

                            Button {
                                text: "◀"
                                font.pixelSize: 14
                                onClicked: viewport.stepBackward()
                            }

                            Button {
                                text: viewport.isPlaying ? "⏸ PAUSE" : "▶ PLAY"
                                font.pixelSize: 13
                                font.bold: true
                                contentItem: Text {
                                    text: parent.text
                                    color: viewport.isPlaying ? "#FFAB00" : "#00E5FF"
                                    font.pixelSize: 13
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: "#1F2333"
                                    radius: 6
                                    border.color: viewport.isPlaying ? "#FFAB00" : "#00E5FF"
                                }
                                onClicked: viewport.togglePlay()
                            }

                            Button {
                                text: "▶"
                                font.pixelSize: 14
                                onClicked: viewport.stepForward()
                            }

                            Button {
                                text: "⏭"
                                font.pixelSize: 14
                                onClicked: viewport.seek(viewport.duration)
                            }
                        }
                    }
                }
            }

            // ---------------- RIGHT PANEL: DAG & FX INSPECTOR ----------------
            Rectangle {
                Layout.preferredWidth: 320
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
                    contentWidth: parent.width
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        anchors.margins: 14
                        spacing: 16

                        Text {
                            text: "DAG NODE COMPOSITOR"
                            color: "#7E849E"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        // DAG Visual Pipeline Card
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 110
                            radius: 8
                            color: "#181A24"
                            border.color: "#26293A"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 6

                                Text {
                                    text: "Active DAG Topology:"
                                    color: "#A0A5B8"
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    spacing: 4
                                    Rectangle {
                                        width: 58; height: 26; radius: 4; color: "#2B263E"; border.color: "#7C4DFF"
                                        Text { anchors.centerIn: parent; text: "ClipNode"; color: "#D1C4E9"; font.pixelSize: 10 }
                                    }
                                    Text { text: "➔"; color: "#545B7A" }
                                    Rectangle {
                                        width: 68; height: 26; radius: 4; color: "#1E303E"; border.color: "#00E5FF"
                                        Text { anchors.centerIn: parent; text: "Transform"; color: "#B2EBF2"; font.pixelSize: 10 }
                                    }
                                    Text { text: "➔"; color: "#545B7A" }
                                    Rectangle {
                                        width: 58; height: 26; radius: 4; color: "#203A2E"; border.color: "#00E676"
                                        Text { anchors.centerIn: parent; text: "EffectNode"; color: "#B9F6CA"; font.pixelSize: 10 }
                                    }
                                    Text { text: "➔"; color: "#545B7A" }
                                    Rectangle {
                                        width: 42; height: 26; radius: 4; color: "#3B2822"; border.color: "#FF9100"
                                        Text { anchors.centerIn: parent; text: "Out"; color: "#FFE0B2"; font.pixelSize: 10 }
                                    }
                                }

                                Text {
                                    text: "Zero-cycle verified • Topological depth: 4"
                                    color: "#00E676"
                                    font.pixelSize: 10
                                }
                            }
                        }

                        // 2D Transform Controls
                        Text {
                            text: "TRANSFORM PROPERTIES"
                            color: "#7E849E"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Text { text: "Scale: " + viewport.scaleFactor.toFixed(2) + "x"; color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: 0.2; to: 2.0; value: viewport.scaleFactor
                                    onMoved: viewport.scaleFactor = value
                                }
                            }

                            RowLayout {
                                Text { text: "Rotation: " + viewport.rotationAngle.toFixed(0) + "°"; color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: -180.0; to: 180.0; value: viewport.rotationAngle
                                    onMoved: viewport.rotationAngle = value
                                }
                            }

                            RowLayout {
                                Text { text: "Opacity: " + Math.round(viewport.opacityValue * 100) + "%"; color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: 0.0; to: 1.0; value: viewport.opacityValue
                                    onMoved: viewport.opacityValue = value
                                }
                            }

                            RowLayout {
                                Text { text: "Position X: " + viewport.posX.toFixed(0) + "px"; color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: -400.0; to: 400.0; value: viewport.posX
                                    onMoved: viewport.posX = value
                                }
                            }

                            RowLayout {
                                Text { text: "Position Y: " + viewport.posY.toFixed(0) + "px"; color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: -300.0; to: 300.0; value: viewport.posY
                                    onMoved: viewport.posY = value
                                }
                            }
                        }

                        // Color Grade & Effect Controls
                        Text {
                            text: "COLOR & FX CONTROLS"
                            color: "#7E849E"
                            font.pixelSize: 11
                            font.bold: true
                            font.letterSpacing: 1.0
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Text { text: "Brightness: " + viewport.brightness.toFixed(2); color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: -0.5; to: 0.5; value: viewport.brightness
                                    onMoved: viewport.brightness = value
                                }
                            }

                            RowLayout {
                                Text { text: "Contrast: " + viewport.contrast.toFixed(2); color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: 0.2; to: 2.0; value: viewport.contrast
                                    onMoved: viewport.contrast = value
                                }
                            }

                            RowLayout {
                                Text { text: "Saturation: " + viewport.saturation.toFixed(2); color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                Slider {
                                    from: 0.0; to: 2.5; value: viewport.saturation
                                    onMoved: viewport.saturation = value
                                }
                            }

                            RowLayout {
                                Text { text: "Blend Mode"; color: "#E0E3EB"; font.pixelSize: 12; Layout.fillWidth: true }
                                ComboBox {
                                    model: ["Normal", "Add", "Multiply", "Screen", "Overlay"]
                                    currentIndex: viewport.blendModeIndex
                                    onActivated: viewport.blendModeIndex = index
                                }
                            }
                        }
                    }
                }
            }
        }

        // ==================== BOTTOM PANEL: MULTI-TRACK TIMELINE ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 180
            color: "#111218"

            Rectangle {
                anchors.top: parent.top
                width: parent.width
                height: 1
                color: "#222533"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Timecode Ruler / Scrubber Bar
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: "#181A22"

                    Slider {
                        id: timelineScrubber
                        anchors.fill: parent
                        anchors.leftMargin: 100
                        anchors.rightMargin: 20
                        from: 0.0
                        to: Math.max(0.1, viewport.duration)
                        value: viewport.position
                        onMoved: viewport.seek(value)
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: "TIMELINE"
                        color: "#7E849E"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                // Track Rows
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#0F1015"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        // Track V2
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            color: "#161821"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 8

                                Rectangle {
                                    width: 70; height: 30; radius: 3; color: "#222636"
                                    Text { anchors.centerIn: parent; text: "V2 (FX)"; color: "#A0A5B8"; font.pixelSize: 11; font.bold: true }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                    radius: 3
                                    color: "#263238"
                                    border.color: "#00E5FF"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "◆ Bézier Keyframe Motion Track"
                                        color: "#80DEEA"
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }

                        // Track V1 (Main Video)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            color: "#161821"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 8

                                Rectangle {
                                    width: 70; height: 30; radius: 3; color: "#222636"
                                    Text { anchors.centerIn: parent; text: "V1 (Video)"; color: "#A0A5B8"; font.pixelSize: 11; font.bold: true }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                    radius: 3
                                    color: "#1E2A38"
                                    border.color: "#3F51B5"
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        Text {
                                            text: viewport.source !== "" ? viewport.source.split('/').pop() : "Synthetic Source Clip"
                                            color: "#C5CAE9"
                                            font.pixelSize: 11
                                            font.bold: true
                                        }
                                    }
                                }
                            }
                        }

                        // Track A1 (Audio Track)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 38
                            color: "#161821"
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 8

                                Rectangle {
                                    width: 70; height: 30; radius: 3; color: "#222636"
                                    Text { anchors.centerIn: parent; text: "A1 (Audio)"; color: "#A0A5B8"; font.pixelSize: 11; font.bold: true }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 28
                                    radius: 3
                                    color: "#1B2E24"
                                    border.color: "#2E7D32"
                                    Text {
                                        anchors.centerIn: parent
                                        text: " ∿ ∿ ∿ ∿ ∿ ∿ ∿ ∿ ∿ ∿ 48kHz Stereo Waveform ∿ ∿ ∿ ∿ ∿ ∿ ∿ ∿ ∿ ∿"
                                        color: "#A5D6A7"
                                        font.pixelSize: 10
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Keyboard Shortcuts
    Shortcut {
        sequence: "Space"
        onActivated: viewport.togglePlay()
    }
    Shortcut {
        sequence: "Left"
        onActivated: viewport.stepBackward()
    }
    Shortcut {
        sequence: "Right"
        onActivated: viewport.stepForward()
    }
    Shortcut {
        sequence: "Home"
        onActivated: viewport.seek(0.0)
    }
    Shortcut {
        sequence: "End"
        onActivated: viewport.seek(viewport.duration)
    }
}
