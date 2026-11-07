import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: win; width: 1200; height: 760; minimumWidth: 840; minimumHeight: 580; visible: true
    title: qsTr("Weatherdeck"); color: "#111114"
    Material.theme: Material.Dark; Material.accent: theme.accent; Material.roundedScale: theme.radius
    property bool searchOpen: false; property bool helpOpen: false
    // Contrast-checked text tones (see critique: secondary metrics must stay legible on the dark column backgrounds)
    property color cDim: "#b8b0b6"      // secondary metrics over dark columns
    property color cHint: "#a49ca2"     // muted labels: footer, hints, timestamps
    property color cAccentStr: "#e4a0a5" // contrast-safe accent pink for small data text
    Shortcut { sequence: "/"; context: Qt.ApplicationShortcut; onActivated: win.searchOpen = true }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: backend.refresh() }
    Shortcut { sequence: "C"; context: Qt.ApplicationShortcut; onActivated: backend.toggleUnits() }
    Shortcut { sequence: "F"; context: Qt.ApplicationShortcut; onActivated: backend.toggleUnits() }
    Shortcut { sequence: "P"; context: Qt.ApplicationShortcut; onActivated: backend.togglePin() }
    Shortcut { sequence: "Tab"; context: Qt.ApplicationShortcut; onActivated: backend.cyclePin(1) }
    Shortcut { sequence: "Shift+Tab"; context: Qt.ApplicationShortcut; onActivated: backend.cyclePin(-1) }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.helpOpen = !win.helpOpen }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    function bar(value, max) { var n=Math.round(Math.min(1,value/max)*8); return "█".repeat(n)+"░".repeat(8-n) }
    component Frame: Rectangle { color: "#17171c"; border.color: "#c47b82"; border.width: 1; radius: Math.min(4, theme.radius); default property alias content: slot.data; Item { id: slot; anchors.fill: parent; anchors.margins: 16 } }
    component LabelValue: Row { required property string label; required property string value; spacing: 6; Text { text: label; color: "#bdb7bd"; font.family: "monospace"; font.pixelSize: 13 } Text { text: "........................"; color: "#51474c"; font.family: "monospace"; font.pixelSize: 13; elide: Text.ElideRight; width: Math.max(20, parent.width - 165) } Text { text: value; color: "#f1ddd8"; font.family: "monospace"; font.pixelSize: 13 } }
    ColumnLayout { anchors.fill: parent; anchors.margins: 18; spacing: 12
        RowLayout { Layout.fillWidth: true; Text { text: "◈ WEATHERDECK"; color: "#e4a0a5"; font.bold: true; font.family: "monospace"; font.pixelSize: 17 } Item { Layout.fillWidth: true } Text { text: backend.dashboard.locationName || qsTr("No location selected"); color: "#f4e9e5"; font.family: "monospace" } Text { text: backend.dashboard.updated ? qsTr("updated %1").arg(backend.dashboard.updated) : ""; color: cHint; font.family: "monospace" } Text { text: backend.dashboard.fahrenheit ? "°F / mph" : "°C / km/h"; color: "#d3ab68"; font.family: "monospace" } }
        Text { visible: backend.dashboard.status.length > 0; text: backend.dashboard.status; color: "#d3ab68"; font.family: "monospace"; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
            Frame { Layout.preferredWidth: 355; Layout.fillHeight: true
                Column { anchors.fill: parent; spacing: 14
                    Text { text: qsTr("CURRENT CONDITIONS"); color: "#e4a0a5"; font.bold: true; font.family: "monospace" }
                    Row { spacing: 15; Text { text: backend.dashboard.current.valid ? backend.dashboard.current.glyph : "…"; color: "#d3ab68"; font.pixelSize: 58 } Column { Text { text: backend.dashboard.current.valid ? Number(backend.dashboard.current.temp).toFixed(0)+backend.dashboard.current.tempUnit : "--"; color: "#f7f1ee"; font.pixelSize: 42; font.family: "monospace" } Text { text: backend.dashboard.current.valid ? backend.dashboard.current.condition : qsTr("Loading"); color: "#bdb7bd"; font.family: "monospace" } } }
                    LabelValue { width: parent.width; label: qsTr("FEELS LIKE"); value: backend.dashboard.current.valid ? Number(backend.dashboard.current.feels).toFixed(0)+backend.dashboard.current.tempUnit : "--" }
                    LabelValue { width: parent.width; label: qsTr("WIND"); value: backend.dashboard.current.valid ? Number(backend.dashboard.current.wind).toFixed(0)+" "+backend.dashboard.current.windUnit : "--" }
                    LabelValue { width: parent.width; label: qsTr("HUMIDITY"); value: backend.dashboard.current.valid ? win.bar(backend.dashboard.current.humidity,100)+" "+backend.dashboard.current.humidity+"%" : "--" }
                    LabelValue { width: parent.width; label: qsTr("PRECIP"); value: backend.dashboard.current.valid ? Number(backend.dashboard.current.rain).toFixed(1)+" mm" : "--" }
                    Rectangle { width: parent.width; height: 1; color: "#51474c" }
                    Text { text: qsTr("ENVIRONMENT"); color: "#e4a0a5"; font.bold: true; font.family: "monospace" }
                    LabelValue { width: parent.width; label: qsTr("AIR QUALITY"); value: backend.dashboard.airQuality.available ? Math.round(backend.dashboard.airQuality.aqi)+" AQI" : "--" }
                    LabelValue { width: parent.width; label: "PM2.5"; value: backend.dashboard.airQuality.available ? Number(backend.dashboard.airQuality.pm25).toFixed(1)+" µg/m³" : "--" }
                }
            }
            ColumnLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                Frame { Layout.fillWidth: true; Layout.preferredHeight: 290
                    Column { anchors.fill: parent; spacing: 10; Text { text: qsTr("NEXT 24 HOURS"); color: "#e4a0a5"; font.bold: true; font.family: "monospace" }
                        ListView { width: parent.width; height: 220; orientation: ListView.Horizontal; spacing: 8; clip: true; model: backend.dashboard.hourly
                            delegate: Rectangle { width: 45; height: 210; color: index % 2 ? "#211b2d" : "#17251f"; radius: Math.min(3,theme.radius)
                                Column { anchors.centerIn: parent; spacing: 7; Text { text: modelData.time; color: cDim; font.family: "monospace"; font.pixelSize: 11 } Text { text: modelData.glyph; color: "#d3ab68"; font.pixelSize: 19 } Rectangle { width: 12; height: Math.max(5, Math.min(95, modelData.rain)); color: "#c47b82"; anchors.horizontalCenter: parent.horizontalCenter; anchors.topMargin: 4 } Text { text: Number(modelData.temp).toFixed(0)+backend.dashboard.current.tempUnit; color: "#f1ddd8"; font.family: "monospace"; font.pixelSize: 11 } Text { text: modelData.rain+"%"; color: cDim; font.family: "monospace"; font.pixelSize: 11 } }
                            }
                        }
                    }
                }
                Frame { Layout.fillWidth: true; Layout.fillHeight: true
                    Column { anchors.fill: parent; spacing: 10; Text { text: qsTr("7-DAY OUTLOOK"); color: "#e4a0a5"; font.bold: true; font.family: "monospace" }
                        ListView { width: parent.width; height: parent.height-30; orientation: ListView.Horizontal; spacing: 9; model: backend.dashboard.daily
                            delegate: Rectangle { width: Math.max(92, (ListView.view.width-54)/7); height: parent.height; color: index % 2 ? "#211b2d" : "#17251f"; radius: Math.min(3,theme.radius)
                                Column { anchors.centerIn: parent; spacing: 7; Text { text: Qt.formatDate(new Date(modelData.date+"T12:00:00"), "ddd"); color: "#c9c1c7"; font.family: "monospace" } Text { text: modelData.glyph; color: "#d3ab68"; font.pixelSize: 24 } Text { text: Number(modelData.max).toFixed(0)+"°"; color: "#f4e9e5"; font.family: "monospace" } Text { text: Number(modelData.min).toFixed(0)+"°"; color: cDim; font.family: "monospace" } Text { text: modelData.rain+"% rain"; color: cAccentStr; font.family: "monospace"; font.pixelSize: 11 } Text { text: "UV "+Number(modelData.uv).toFixed(0); color: "#d3ab68"; font.family: "monospace"; font.pixelSize: 11 } }
                            }
                        }
                    }
                }
            }
        }
        Text { Layout.fillWidth: true; text: "[ / ] "+qsTr("Search")+"   [ r ] "+qsTr("Refresh")+"   [ c/f ] "+qsTr("Toggle units")+"   [ p ] "+(backend.dashboard.pinned ? qsTr("Unpin") : qsTr("Pin"))+"   [ Tab ] "+qsTr("Locations")+"   [ ? ] "+qsTr("Help")+"   [ q ] "+qsTr("Quit"); color: cHint; font.family: "monospace"; horizontalAlignment: Text.AlignHCenter }
    }
    Rectangle { visible: searchOpen; anchors.fill: parent; color: "#000000aa"; z: 5; focus: visible
        Frame { width: Math.min(620,parent.width-50); height: Math.min(480,parent.height-70); anchors.centerIn: parent; color: "#1c1a20"
            Column { anchors.fill: parent; spacing: 14; Text { text: qsTr("SEARCH LOCATION"); color: "#e4a0a5"; font.bold: true; font.family: "monospace" }
                TextField { id: searchField; width: parent.width; placeholderText: qsTr("City or postal code…"); color: "#f5eeeb"; focus: searchOpen; onTextChanged: backend.search(text); Keys.onReturnPressed: { if (backend.searchResults.length) backend.selectSearchResult(0); win.searchOpen=false }
                    Keys.onEscapePressed: win.searchOpen = false
                    background: Rectangle { color: "#111114"; radius: Math.min(3, theme.radius); border.width: 1; border.color: searchField.activeFocus ? theme.accent : "#51474c" } }
                ListView { width: parent.width; height: parent.height-140; model: backend.searchResults; clip: true; delegate: ItemDelegate { width: parent.width; text: modelData.name+", "+modelData.country+(modelData.admin ? " · "+modelData.admin : ""); onClicked: { backend.selectSearchResult(index); win.searchOpen=false } } }
                Text { text: qsTr("Enter to search · Esc to close"); color: cHint; font.family: "monospace" }
            }
        }
        Keys.onEscapePressed: searchOpen=false
    }
    Rectangle { visible: helpOpen; anchors.fill: parent; color: "#000000bb"; z: 6; focus: visible
        Frame { width: 470; height: 300; anchors.centerIn: parent; Column { anchors.fill: parent; spacing: 10; Text { text: qsTr("KEYBOARD SHORTCUTS"); color: "#e4a0a5"; font.bold: true; font.family: "monospace" } Text { text: "[ / ]  "+qsTr("Search location")+"\n[ r ]  "+qsTr("Refresh weather")+"\n[ c/f ]  "+qsTr("Toggle units")+"\n[ p ]  "+qsTr("Pin current city")+"\n[ Tab ]  "+qsTr("Cycle pinned cities")+"\n[ q ]  "+qsTr("Quit"); color: "#f1ddd8"; font.family: "monospace"; lineHeight: 1.6 } Text { text: qsTr("Press ? or Esc to close"); color: cHint; font.family: "monospace" } } }
        Keys.onEscapePressed: helpOpen=false; Keys.onPressed: event => { if (event.key === Qt.Key_Question) helpOpen=false }
    }
}
