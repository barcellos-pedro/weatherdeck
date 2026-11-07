import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1240; height: 740; minimumWidth: 1020; minimumHeight: 700; visible: true
    title: qsTr("Weatherdeck"); color: theme.bg
    Material.theme: Material.Dark; Material.accent: theme.accent
    property bool helpOpen: false
    readonly property bool dataReady: backend.dashboard.current.valid

    // Derived strips recomputed on every dashboard state change
    property var hourList: []
    property real sparkMin: 0
    property real sparkMax: 1
    property real weekMin: 0
    property real weekRange: 1

    function sparkY(value, height) {
        const pad = 8
        const rg = Math.max(1.0, sparkMax - sparkMin)
        if (sparkMax - sparkMin < 1.0) {
            const mid = height / 2
            return mid
        }
        return pad + (1 - (value - sparkMin) / rg) * (height - 2 * pad)
    }

    Shortcut { sequence: "/"; context: Qt.ApplicationShortcut; enabled: !searchField.activeFocus; onActivated: searchField.forceActiveFocus() }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: backend.refresh() }
    Shortcut { sequence: "C"; context: Qt.ApplicationShortcut; onActivated: backend.toggleUnits() }
    Shortcut { sequence: "F"; context: Qt.ApplicationShortcut; onActivated: backend.toggleUnits() }
    Shortcut { sequence: "P"; context: Qt.ApplicationShortcut; onActivated: backend.togglePin() }
    Shortcut { sequence: "Tab"; context: Qt.ApplicationShortcut; onActivated: backend.cyclePin(1) }
    Shortcut { sequence: "Shift+Tab"; context: Qt.ApplicationShortcut; onActivated: backend.cyclePin(-1) }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpOpen = !win.helpOpen }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }

    // ---------- helpers ----------
    function relTime(epoch) {
        if (!epoch) return ""
        const mins = Math.max(0, Math.round((Date.now() / 1000 - epoch) / 60))
        if (mins < 1) return qsTr("just now")
        if (mins < 60) return qsTr("%1m ago").arg(mins)
        const h = Math.floor(mins / 60), m = mins % 60
        if (h < 24) return m ? qsTr("%1h %2m ago").arg(h).arg(m) : qsTr("%1h ago").arg(h)
        return qsTr("updated %1").arg(backend.dashboard.updated)
    }
    function bandLabel(key) {
        if (key === "fair") return qsTr("Fair")
        if (key === "moderate") return qsTr("Moderate")
        if (key === "poor") return qsTr("Poor")
        if (key === "very-poor") return qsTr("Very poor")
        if (key === "extreme") return qsTr("Extreme")
        return qsTr("Good")
    }
    function bandColor(key) {
        if (key === "fair") return theme.fair
        if (key === "moderate") return theme.warn
        if (key === "poor") return theme.poor
        if (key === "very-poor" || key === "extreme") return theme.bad
        return theme.good
    }
    function popColor(v) { return v >= 40 ? theme.warn : v >= 15 ? theme.fair : theme.textFaint }
    function refreshDerived() {
        const list = backend.dashboard.hourly
        hourList = list.slice(0, 12)
        if (hourList.length) {
            let mn = hourList[0].temp, mx = hourList[0].temp
            for (let i = 1; i < hourList.length; i++) { mn = Math.min(mn, hourList[i].temp); mx = Math.max(mx, hourList[i].temp) }
            sparkMin = mn; sparkMax = mx
        } else { sparkMin = 0; sparkMax = 1 }
        const d = backend.dashboard.daily
        if (d.length) {
            let mn = d[0].min, mx = d[0].max
            for (let i = 0; i < d.length; i++) { mn = Math.min(mn, d[i].min); mx = Math.max(mx, d[i].max) }
            weekMin = mn
            weekRange = Math.max(1, mx - mn)
        } else { weekMin = 0; weekRange = 1 }
    }

    // ---------- shared components ----------
    component Panel: Rectangle {
        default property alias content: slot.data
        color: theme.surface; radius: theme.radius
        border.color: theme.border; border.width: 1
        Item { id: slot; anchors.fill: parent; anchors.margins: 14; clip: true }
    }
    component PanelTitle: Text {
        font.family: "monospace"; font.pixelSize: 12; font.bold: true
        color: theme.textFaint; font.letterSpacing: 2
    }
    component PixelIcon: Image {
        property string iconKey
        onIconKeyChanged: if (iconKey) source = "qrc:/icons/weather-" + iconKey + ".png"
        asynchronous: true; smooth: false; mipmap: false
        fillMode: Image.PreserveAspectFit
    }
    // Numbered pinned-city chip: click to activate, ✕ button to unpin
    component PinChip: Rectangle {
        id: chip
        required property var modelData
        // Guarded while the model refills (modelData can be undefined briefly)
        readonly property var d: modelData ? modelData : {}
        readonly property bool active: backend.dashboard.activePinIndex === d.index
        readonly property bool hover: chipArea.containsMouse
        implicitWidth: chipRow.implicitWidth + 24
        implicitHeight: 26
        radius: 13
        color: active ? theme.surfaceAlt : hover ? theme.bg : "transparent"
        border.color: active ? theme.accent : theme.border
        border.width: active ? 2 : 1
        Row {
            id: chipRow
            anchors.centerIn: parent; spacing: 7
            Text {
                text: chip.d.index !== undefined ? "[" + (chip.d.index + 1) + "]" : ""
                color: chip.active ? theme.accent : theme.textFaint
                font.family: "monospace"; font.pixelSize: 12; font.bold: chip.active
            }
            Text {
                text: chip.d.name ? chip.d.name.toUpperCase() + (chip.d.country ? ", " + chip.d.country.toUpperCase() : "") : ""
                color: chip.active ? theme.textBright : theme.textDim
                font.family: "monospace"; font.pixelSize: 12
            }
            Text {
                visible: !chip.active || chip.hover
                text: "✕"
                color: hoverArea.containsMouse ? theme.bad : theme.textFaint
                font.family: "monospace"; font.pixelSize: 12
                leftPadding: 3; rightPadding: 3
                MouseArea {
                    id: hoverArea
                    hoverEnabled: true
                    anchors.fill: parent
                    anchors.margins: -4
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (chip.d.index !== undefined) backend.unpinAt(chip.d.index)
                }
            }
        }
        MouseArea {
            id: chipArea
            hoverEnabled: true
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: if (chip.d.index !== undefined) backend.selectPin(chip.d.index)
        }
    }
    component Metric: Column {
        property string label; property string value; property color valueColor: theme.textBright
        spacing: 4
        Text { text: parent.label; color: theme.textFaint; font.family: "monospace"; font.pixelSize: 12 }
        Text { text: parent.value; color: parent.valueColor; font.family: "monospace"; font.pixelSize: 16 }
    }
    component LegendKey: Rectangle {
        property string label
        implicitWidth: keyText.implicitWidth + 8
        implicitHeight: 17
        color: theme.border; radius: 2
        Text {
            id: keyText
            anchors.centerIn: parent
            font.family: "monospace"; font.pixelSize: 11; color: theme.bg
            text: parent.label
        }
    }

    Connections {
        target: backend.dashboard
        function onChanged() { win.refreshDerived() }
    }
    Component.onCompleted: win.refreshDerived()

    Timer { interval: 30000; repeat: true; running: true; onTriggered: footerTimeText.text = win.relTime(backend.dashboard.updatedAt) }

    ColumnLayout {
        id: mainCol
        anchors.fill: parent; anchors.margins: 14; spacing: 10

        // ================= HEADER =================
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            ColumnLayout { spacing: 0
                Text { text: "WEATHERDECK"; color: theme.textBright; font.family: "monospace"; font.pixelSize: 19; font.bold: true; font.letterSpacing: 1 }
                Text { text: qsTr("by Omarchy"); color: theme.poor; font.family: "monospace"; font.pixelSize: 11 }
            }
            Item { Layout.fillWidth: true }
            ColumnLayout { spacing: 4
                TextField {
                    id: searchField
                    Layout.preferredWidth: 300
                    placeholderText: "[ / ] " + qsTr("Search city…")
                    color: theme.textBright; font.family: "monospace"; font.pixelSize: 12
                    selectByMouse: true
                    onTextChanged: backend.search(text)
                    Keys.onReturnPressed: { if (backend.searchResults.length) { win.selectSearch(0); } }
                    Keys.onEscapePressed: text = ""
                    background: Rectangle {
                        color: theme.bg; radius: theme.radius
                        border.color: searchField.activeFocus ? theme.accent : theme.border
                        border.width: searchField.activeFocus ? 2 : 1
                    }
                    // Results dropdown
                    Rectangle {
                        visible: backend.searchResults.length > 0 && searchField.text.length >= 2
                        width: parent.width; y: parent.height + 6; z: 60
                        height: Math.min(280, backend.searchResults.length * 34 + 16)
                        color: theme.surface; radius: theme.radius
                        border.color: theme.accent; border.width: 1
                        ListView {
                            id: resultsList
                            anchors.fill: parent; anchors.margins: 8; clip: true
                            model: backend.searchResults
                            delegate: Rectangle {
                                width: resultsList.width; height: 30
                                color: resultHover.containsMouse ? theme.surfaceAlt : "transparent"
                                radius: 2
                                RowLayout { anchors.fill: parent; spacing: 4
                                    Text { text: modelData.name; color: theme.textBright; font.family: "monospace"; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
                                    Text { text: modelData.country; color: theme.textFaint; font.family: "monospace"; font.pixelSize: 11; visible: modelData.country.length > 0 }
                                }
                                MouseArea { id: resultHover; hoverEnabled: true; anchors.fill: parent; onClicked: win.selectSearch(index) }
                            }
                            Text { visible: backend.searchResults.length === 0; anchors.centerIn: parent; text: qsTr("No results"); color: theme.textFaint; font.family: "monospace"; font.pixelSize: 12 }
                        }
                    }
                }
                Text {
                    id: headerStatus
                    color: backend.dashboard.status.length ? theme.warn : theme.textFaint
                    font.family: "monospace"; font.pixelSize: 11
                    Layout.preferredWidth: 300; horizontalAlignment: Text.AlignRight
                    elide: Text.ElideMiddle
                    text: {
                        var dot = backend.dashboard.status.length ? "●" : backend.dashboard.loading ? "◐" : "●"
                        dot = backend.dashboard.status.length ? "▲" : dot
                        var base = backend.dashboard.status.length ? backend.dashboard.status : backend.dashboard.loading ? qsTr("Refreshing…") : qsTr("Connected to Open-Meteo")
                        var stamp = win.dataReady ? " • " + win.relTime(backend.dashboard.updatedAt) + " [r]" : ""
                        return dot + " " + base + stamp
                    }
                }
            }
        }

        // ================= PINNED PLACES =================
        Rectangle {
            Layout.fillWidth: true; implicitHeight: 42
            color: theme.surface; radius: theme.radius
            border.color: theme.border; border.width: 1
            RowLayout { anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 10
                Text { text: qsTr("Pinned Places"); color: theme.textFaint; font.family: "monospace"; font.pixelSize: 12; font.bold: true; font.letterSpacing: 1 }
                LegendKey { label: "Tab" }
                Text {
                    Layout.fillWidth: true; visible: backend.dashboard.pins.length === 0
                    text: qsTr("No pins yet — press [P] to pin the current city, [Tab] to cycle pins")
                    color: theme.textFaint; font.family: "monospace"; font.pixelSize: 12
                    elide: Text.ElideRight
                }
                Row { Layout.fillWidth: true; spacing: 8; visible: backend.dashboard.pins.length > 0
                    Repeater {
                        model: backend.dashboard.pins
                        delegate: PinChip {}
                    }
                }
            }
        }

        // ================= PRIMARY CONTENT =================
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10

            // ==== Left column ====
            ColumnLayout { Layout.preferredWidth: 340; Layout.maximumWidth: 380; Layout.minimumWidth: 300; Layout.fillHeight: true; spacing: 10

                Panel { Layout.fillWidth: true; Layout.fillHeight: true
                    ColumnLayout { anchors.fill: parent; spacing: 8
                        PanelTitle { text: qsTr("ACTIVE LOCATION") }
                        RowLayout { Layout.fillWidth: true; spacing: 6
                            Text {
                                Layout.fillWidth: true
                                text: win.dataReady ? backend.dashboard.locationName : qsTr("No location selected")
                                color: theme.textBright; font.family: "monospace"; font.pixelSize: 17; font.bold: true
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: win.dataReady
                                text: "(" + backend.dashboard.coordinates + ")"
                                color: theme.textFaint; font.family: "monospace"; font.pixelSize: 11
                                elide: Text.ElideMiddle; Layout.maximumWidth: 140
                            }
                        }
                        Item { Layout.fillHeight: true }
                        RowLayout { Layout.fillWidth: true; spacing: 8
                            Text {
                                Layout.alignment: Qt.AlignVCenter
                                text: win.dataReady ? Number(backend.dashboard.current.temp).toFixed(0) + backend.dashboard.current.tempUnit : "--"
                                color: theme.textBright; font.family: "monospace"; font.pixelSize: 54; font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            PixelIcon {
                                Layout.preferredHeight: 76; Layout.preferredWidth: 76
                                Layout.alignment: Qt.AlignVCenter
                                iconKey: win.dataReady ? backend.dashboard.current.iconKey : ""
                                visible: iconKey.length > 0
                            }
                        }
                        Text {
                            text: backend.dashboard.current.condition
                            visible: win.dataReady
                            color: theme.textDim; font.family: "monospace"; font.pixelSize: 13
                        }
                        Item { Layout.fillHeight: true }
                        GridLayout { Layout.fillWidth: true; columns: 2; columnSpacing: 16; rowSpacing: 8
                            Metric { label: qsTr("Apparent Temp"); value: win.dataReady ? Number(backend.dashboard.current.feels).toFixed(0) + backend.dashboard.current.tempUnit : "--" }
                            Metric { label: qsTr("Humidity"); value: win.dataReady ? Number(backend.dashboard.current.humidity).toFixed(0) + "%" : "--" }
                            Metric { label: qsTr("Precipitation"); value: win.dataReady ? Number(backend.dashboard.current.rain).toFixed(1) + " mm" : "--" }
                            Metric { label: qsTr("UV Index"); value: win.dataReady ? Number(backend.dashboard.current.uv).toFixed(0) + (backend.dashboard.current.isDay ? "" : " " + qsTr("[Night]")) : "--" }
                        }
                        Rectangle { Layout.fillWidth: true; height: 1; color: theme.border }
                        RowLayout { Layout.fillWidth: true; spacing: 8
                            Rectangle {
                                implicitHeight: 26; implicitWidth: pinnedRow.implicitWidth + 18; radius: 13
                                color: backend.dashboard.pinned ? theme.surfaceAlt : "transparent"
                                border.color: backend.dashboard.pinned ? theme.accent : theme.border; border.width: 1
                                Row {
                                    id: pinnedRow; anchors.centerIn: parent; spacing: 7
                                    Image {
                                        width: 12; height: 12; anchors.verticalCenter: parent.verticalCenter
                                        smooth: false; source: "qrc:/icons/pin.png"; visible: backend.dashboard.pinned
                                    }
                                    Text { text: backend.dashboard.pinned ? qsTr("pinned") : qsTr("unpinned"); color: backend.dashboard.pinned ? theme.accent : theme.textFaint; font.family: "monospace"; font.pixelSize: 12 }
                                }
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: "[P] " + (backend.dashboard.pinned ? qsTr("Unpin") : qsTr("Pin"))
                                color: theme.textDim; font.family: "monospace"; font.pixelSize: 12
                                TapHandler { onTapped: backend.togglePin() }
                            }
                        }
                    }
                }

                Panel { Layout.fillWidth: true; Layout.preferredHeight: 176
                    ColumnLayout { anchors.fill: parent; spacing: 8
                        PanelTitle { text: qsTr("AIR QUALITY") }
                        Text {
                            visible: !backend.dashboard.airQuality.available
                            text: qsTr("Air quality data not available")
                            color: theme.textFaint; font.family: "monospace"; font.pixelSize: 13
                        }
                        ColumnLayout {
                            id: aqCol
                            visible: backend.dashboard.airQuality.available; Layout.fillWidth: true; spacing: 8
                            property string band: backend.dashboard.airQuality.available ? backend.airQualityBandKey(backend.dashboard.airQuality.aqi) : ""
                            readonly property color bandCol: bandColor(band)
                            RowLayout { Layout.fillWidth: true; spacing: 6
                                Text { text: "EU AQI:"; color: theme.textDim; font.family: "monospace"; font.pixelSize: 15 }
                                Text { text: backend.dashboard.airQuality.available ? Math.round(backend.dashboard.airQuality.aqi) : "--"; color: aqCol.bandCol; font.family: "monospace"; font.pixelSize: 22; font.bold: true }
                                Text { text: aqCol.band ? "(" + bandLabel(aqCol.band) + ")" : ""; color: aqCol.bandCol; font.family: "monospace"; font.pixelSize: 14 }
                                Item { Layout.fillWidth: true }
                            }
                            // EU AQI band gradient with marker at the current value
                            Item {
                                Layout.fillWidth: true; implicitHeight: 12
                                Rectangle
                                {
                                    anchors.fill: parent; radius: 6; color: theme.bg
                                    border.color: theme.border; border.width: 1
                                }
                                Rectangle {
                                    id: aqiFill
                                    property real fraction: backend.dashboard.airQuality.available ? backend.airQualityBarFraction(backend.dashboard.airQuality.aqi) : 0
                                    x: 2; height: parent.height - 4; y: 2
                                    width: Math.max(0, (parent.width - 4) * fraction)
                                    radius: 4
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop { position: 0.0; color: theme.good }
                                        GradientStop { position: 14 / 140; color: theme.good }
                                        GradientStop { position: 30 / 140; color: theme.fair }
                                        GradientStop { position: 50 / 140; color: theme.warn }
                                        GradientStop { position: 70 / 140; color: theme.poor }
                                        GradientStop { position: 90 / 140; color: theme.bad }
                                        GradientStop { position: 1.0; color: theme.extreme }
                                    }
                                }
                            }
                            RowLayout { Layout.fillWidth: true; spacing: 6
                                Text { text: "PM2.5:"; color: theme.textDim; font.family: "monospace"; font.pixelSize: 13 }
                                Text { text: backend.dashboard.airQuality.available ? Number(backend.dashboard.airQuality.pm25).toFixed(1) + " " + qsTr("µg/m³") : "--"; color: theme.textBright; font.family: "monospace"; font.pixelSize: 13 }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }

            // ==== Right column ====
            ColumnLayout { Layout.fillWidth: true; Layout.minimumWidth: 420; Layout.fillHeight: true; spacing: 10

                Panel { Layout.fillWidth: true; Layout.preferredHeight: 306
                    // Plain Column with fixed row heights; avoids nested ColumnLayout sizing quirks
                    Column { anchors.fill: parent; spacing: 8
                        Row {
                            id: hoursTitle
                            spacing: 6
                            PanelTitle { text: qsTr("HOURLY FORECAST") }
                            Text { text: qsTr("(next 12h)"); color: theme.textFaint; font.family: "monospace"; font.pixelSize: 11 }
                        }
                        Item {
                            id: hourHead
                            width: parent.width
                            height: 62
                            visible: win.hourList.length > 0
                            Row {
                                id: hourRow
                                spacing: 0
                                anchors.top: parent.top
                                anchors.left: parent.left; anchors.right: parent.right
                                Repeater {
                                    model: win.hourList
                                    delegate: Column {
                                        width: hourRow.width / 12
                                        spacing: 5
                                        Text { text: modelData.time; color: theme.textFaint; font.family: "monospace"; font.pixelSize: 10; anchors.horizontalCenter: parent.horizontalCenter }
                                        PixelIcon { iconKey: modelData.iconKey; width: 30; height: 30; anchors.horizontalCenter: parent.horizontalCenter }
                                        Text { text: Number(modelData.temp).toFixed(0) + "°"; color: theme.textBright; font.family: "monospace"; font.pixelSize: 12; anchors.horizontalCenter: parent.horizontalCenter }
                                    }
                                }
                            }
                        }
                        // Temperature polyline drawn from segment rectangles (picks from win.spark* bounds)
                        Item {
                            id: sparkArea
                            visible: win.hourList.length > 1
                            width: parent.width
                            height: Math.max(40, parent.height - hoursTitle.height - hourHead.height - popHead.height - 3 * 8)
                            Repeater {
                                model: win.hourList.length > 1 ? win.hourList.length - 1 : 0
                                Rectangle {
                                    color: theme.accent
                                    height: 2
                                    transformOrigin: Item.Left
                                    visible: index < win.hourList.length - 1
                                    property real iw: sparkArea.width / win.hourList.length
                                    property var a: win.hourList[index]
                                    property var b: win.hourList[index + 1]
                                    property real x1: a ? (index + 0.5) * iw : 0
                                    property real x2: b ? (index + 1.5) * iw : 0
                                    property real y1: a ? win.sparkY(a.temp, sparkArea.height) : 0
                                    property real y2: b ? win.sparkY(b.temp, sparkArea.height) : y1
                                    x: x1; y: y1 - 1
                                    width: Math.max(1, Math.sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)))
                                    rotation: Math.atan2(y2 - y1, x2 - x1) * 180 / Math.PI
                                }
                            }
                            Rectangle {
                                visible: win.hourList.length > 0 && win.hourList[win.hourList.length - 1] !== undefined
                                x: (win.hourList.length - 0.5) * sparkArea.width / win.hourList.length - 3
                                y: win.hourList[win.hourList.length - 1] !== undefined ? win.sparkY(win.hourList[win.hourList.length - 1].temp, sparkArea.height) - 3 : 0
                                width: 6; height: 6; radius: 3
                                color: theme.accent
                            }
                        }
                        Item {
                            id: popHead
                            visible: win.hourList.length > 0
                            width: parent.width
                            height: 36
                            Row {
                                id: popRow
                                spacing: 0
                                anchors.top: parent.top
                                anchors.left: parent.left; anchors.right: parent.right
                                Repeater {
                                    model: win.hourList
                                    delegate: Column {
                                        width: popRow.width / 12
                                        spacing: 2
                                        Image { source: "qrc:/icons/drop.png"; smooth: false; width: 10; height: 10; anchors.horizontalCenter: parent.horizontalCenter; visible: modelData.rain >= 10 }
                                        Text { text: Math.round(modelData.rain) + "%"; color: popColor(modelData.rain); font.family: "monospace"; font.pixelSize: 11; anchors.horizontalCenter: parent.horizontalCenter }
                                    }
                                }
                            }
                        }
                    }
                }

                Panel { Layout.fillWidth: true; Layout.fillHeight: true
                    ColumnLayout { anchors.fill: parent; spacing: 4
                        PanelTitle { text: qsTr("7-DAY OUTLOOK") }
                        Text {
                            visible: backend.dashboard.daily.length === 0
                            text: qsTr("Waiting for 7-day data…")
                            color: theme.textFaint; font.family: "monospace"; font.pixelSize: 13
                        }
                        // Clipped + scrollable so rows never spill past the card
                        ListView {
                            visible: backend.dashboard.daily.length > 0
                            Layout.fillWidth: true; Layout.fillHeight: true
                            clip: true
                            spacing: 3
                            model: backend.dashboard.daily
                            delegate: RowLayout {
                                width: ListView.view.width
                                spacing: 10
                                Text {
                                    text: Qt.formatDate(new Date(modelData.date + "T12:00:00"), "ddd")
                                    color: theme.textDim; font.family: "monospace"; font.pixelSize: 13
                                    Layout.preferredWidth: 44
                                }
                                PixelIcon { iconKey: modelData.iconKey; Layout.preferredWidth: 24; Layout.preferredHeight: 24 }
                                Image { source: "qrc:/icons/drop.png"; smooth: false; visible: modelData.rain >= 10; Layout.preferredWidth: 10; Layout.preferredHeight: 10 }
                                Text {
                                    text: Math.round(modelData.rain) + "%"
                                    visible: modelData.rain >= 10
                                    color: popColor(modelData.rain); font.family: "monospace"; font.pixelSize: 11
                                    Layout.preferredWidth: 34
                                }
                                Text {
                                    text: Number(modelData.min).toFixed(0) + "°"
                                    color: theme.textDim; font.family: "monospace"; font.pixelSize: 13
                                    Layout.alignment: Qt.AlignRight
                                    Layout.preferredWidth: 34
                                }
                                // Temperature range gutter: track + segment positioned by weekly bounds
                                Rectangle {
                                    Layout.fillWidth: true; Layout.preferredHeight: 8
                                    radius: 4; color: theme.bg
                                    border.color: theme.border; border.width: 1
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        x: parent.width * Math.min(1, Math.max(0, (modelData.min - win.weekMin) / win.weekRange))
                                        width: Math.max(6, parent.width * Math.max(0.02, (modelData.max - modelData.min) / win.weekRange))
                                        height: parent.height - 2; radius: 3
                                        gradient: Gradient {
                                            orientation: Gradient.Horizontal
                                            GradientStop { position: 0.0; color: theme.fair }
                                            GradientStop { position: 1.0; color: theme.good }
                                        }
                                    }
                                }
                                Text {
                                    text: Number(modelData.max).toFixed(0) + "°"
                                    color: theme.textBright; font.family: "monospace"; font.pixelSize: 13
                                    Layout.preferredWidth: 34
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle { visible: backend.dashboard.status.length > 0; Layout.fillWidth: true; height: 24; color: theme.surfaceAlt; radius: 3
            Text { anchors.centerIn: parent; text: backend.dashboard.status; color: theme.warn; font.family: "monospace"; font.pixelSize: 12; elide: Text.ElideMiddle; width: parent.width - 20; horizontalAlignment: Text.AlignHCenter }
        }

        // ================= FOOTER =================
        RowLayout {
            Layout.fillWidth: true; spacing: 14
            RowLayout { spacing: 12
                Text { font.family: "monospace"; font.pixelSize: 11; color: theme.textFaint
                    text: "[ / ] " + qsTr("Search") + "   [ R ] " + qsTr("Refresh") + "   [ C ]/[ F ] °C/°F   [ P ] " + qsTr("Pin Location") + "   [ Tab ]/[Shift+Tab] " + qsTr("Cycle Pins") + "   [ ? ] " + qsTr("Help") + "   [ Q ] " + qsTr("Quit")
                }
                Item { Layout.fillWidth: true }
                Text {
                    id: footerTimeText
                    font.family: "monospace"; font.pixelSize: 11
                    color: theme.textFaint
                    text: win.relTime(backend.dashboard.updatedAt)
                }
                Text { font.family: "monospace"; font.pixelSize: 11; color: theme.textFaint
                    text: qsTr("Location: %1 • Generation %2").arg(win.dataReady ? qsTr("Active") : qsTr("None")).arg(backend.dashboard.generation)
                }
            }
        }
    }

    function selectSearch(index) {
        backend.selectSearchResult(index)
        searchField.text = ""
    }

    // ================= HELP OVERLAY =================
    Rectangle {
        visible: helpOpen; anchors.fill: parent; color: "#000000bb"; z: 90
        focus: visible
        Keys.onEscapePressed: helpOpen = false
        Keys.onPressed: event => { if (event.key === Qt.Key_Question) helpOpen = false }
        Panel {
            width: 460; height: 320; anchors.centerIn: parent
            ColumnLayout { anchors.fill: parent; spacing: 10
                PanelTitle { text: qsTr("KEYBOARD SHORTCUTS") }
                Text { text: "[ / ]  " + qsTr("Search location")
                            + "\n[ R ]  " + qsTr("Refresh weather")
                            + "\n[ C ]/[ F ]  " + qsTr("Toggle units")
                            + "\n[ P ]  " + qsTr("Pin current city")
                            + "\n[ Tab ]  " + qsTr("Cycle pinned cities")
                            + "\n[ ? ]  " + qsTr("Help")
                            + "\n[ Q ]  " + qsTr("Quit")
                    color: theme.textBright; font.family: "monospace"; font.pixelSize: 13; lineHeight: 1.5 }
                Item { Layout.fillHeight: true }
                Text { text: qsTr("Press ? or Esc to close"); color: theme.textFaint; font.family: "monospace"; font.pixelSize: 12 }
            }
        }
    }
}
