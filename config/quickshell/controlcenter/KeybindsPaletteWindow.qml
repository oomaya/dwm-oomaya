import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Io
import qs.core

pragma ComponentBehavior: Bound

FloatingWindow {
    id: root

    required property var controlCenterModel

    title: "dwm control center utility - keybinds"
    visible: controlCenterModel.utilityVisible && controlCenterModel.utilityPage === "keybinds"
    screen: controlCenterModel.utilityScreen
    implicitWidth: 840
    implicitHeight: 600
    color: Theme.transparent

    property string query: ""
    property string activeCategory: "All"
    property int selectedIndex: 0

    function focusSearch() {
        searchTextInput.forceActiveFocus();
        searchTextInput.cursorPosition = searchTextInput.text.length;
    }

    onVisibleChanged: {
        if (visible) {
            root.query = "";
            root.activeCategory = "All";
            root.selectedIndex = 0;
            Qt.callLater(root.focusSearch);
        }
    }

    readonly property var availableCategories: {
        const cats = ["All"];
        const rows = root.controlCenterModel.keybindRows || [];
        for (let i = 0; i < rows.length; i++) {
            const cat = rows[i].category;
            if (cat && cat.length > 0 && cats.indexOf(cat) === -1) {
                cats.push(cat);
            }
        }
        return cats;
    }

    readonly property var filteredRows: {
        const rows = root.controlCenterModel.keybindRows || [];
        const q = root.query.trim().toLowerCase();
        const cat = root.activeCategory;
        const result = [];

        for (let i = 0; i < rows.length; i++) {
            const row = rows[i];
            if (cat !== "All" && row.category !== cat) {
                continue;
            }
            if (q.length > 0) {
                const descMatch = (row.description || "").toLowerCase().indexOf(q) !== -1;
                const keyMatch = (row.keys || "").toLowerCase().indexOf(q) !== -1;
                const catMatch = (row.category || "").toLowerCase().indexOf(q) !== -1;
                const funcMatch = (row.func || "").toLowerCase().indexOf(q) !== -1;
                if (!descMatch && !keyMatch && !catMatch && !funcMatch) {
                    continue;
                }
            }
            result.push(row);
        }
        return result;
    }

    function selectRelative(delta) {
        const count = root.filteredRows.length;
        if (count === 0) {
            root.selectedIndex = 0;
            return;
        }
        let next = root.selectedIndex + delta;
        if (next < 0) next = 0;
        if (next >= count) next = count - 1;
        root.selectedIndex = next;
    }

    function selectAbsolute(idx) {
        const count = root.filteredRows.length;
        if (count === 0) {
            root.selectedIndex = 0;
            return;
        }
        if (idx < 0) idx = 0;
        if (idx >= count) idx = count - 1;
        root.selectedIndex = idx;
    }

    function executeSelected() {
        if (root.filteredRows.length === 0) return;
        const index = Math.max(0, Math.min(root.selectedIndex, root.filteredRows.length - 1));
        const row = root.filteredRows[index];
        root.controlCenterModel.executeKeybind(row);
    }

    ShellSurface {
        anchors.fill: parent
        focus: true

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                root.controlCenterModel.closeUtility();
                event.accepted = true;
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacingMd

            // Header Row
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingLg

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingXxs

                    UiText {
                        text: "Keybindings Mastery"
                        color: Theme.textStrong
                        font.pixelSize: Theme.titleFontSize
                        font.bold: true
                    }

                    UiText {
                        text: "Type to search · ↑↓/Ctrl-n/p to navigate · Enter to EXECUTE · Esc to close"
                        color: Theme.textMuted
                        font.pixelSize: Theme.smallFontSize
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: counterText.implicitWidth + Theme.spacingXl * 2
                    color: Theme.controlNormalFill
                    border.color: Theme.controlNormalBorder
                    border.width: 1
                    radius: 0

                    UiText {
                        id: counterText
                        anchors.centerIn: parent
                        text: root.filteredRows.length + " / " + (root.controlCenterModel.keybindRows ? root.controlCenterModel.keybindRows.length : 0) + " keys"
                        color: Theme.accent
                        font.pixelSize: Theme.smallFontSize
                        font.bold: true
                    }
                }

                ShellButton {
                    label: "Close"
                    onActivated: root.controlCenterModel.closeUtility()
                }
            }

            // Search Input Box
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                color: Theme.controlNormalFill
                border.color: searchTextInput.activeFocus ? Theme.accent : Theme.controlNormalBorder
                border.width: searchTextInput.activeFocus ? 2 : 1
                radius: 0

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLg
                    anchors.rightMargin: Theme.spacingLg
                    spacing: Theme.spacingMd

                    UiText {
                        text: "/"
                        color: searchTextInput.activeFocus ? Theme.accent : Theme.textMuted
                        font.pixelSize: Theme.inputFontSize
                        font.bold: true
                    }

                    TextInput {
                        id: searchTextInput

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textStrong
                        selectionColor: Theme.accent
                        selectedTextColor: Theme.accentText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.inputFontSize
                        clip: true
                        text: root.query

                        onTextChanged: {
                            if (text !== root.query) {
                                root.query = text;
                                root.selectedIndex = 0;
                            }
                        }

                        Keys.onPressed: function(event) {
                            const ctrl = event.modifiers & Qt.ControlModifier;

                            if (event.key === Qt.Key_Down || (event.key === Qt.Key_N && ctrl)) {
                                root.selectRelative(1);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Up || (event.key === Qt.Key_P && ctrl)) {
                                root.selectRelative(-1);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_PageDown) {
                                root.selectRelative(6);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_PageUp) {
                                root.selectRelative(-6);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Home) {
                                root.selectAbsolute(0);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_End) {
                                root.selectAbsolute(root.filteredRows.length - 1);
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                root.executeSelected();
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Escape || (event.key === Qt.Key_C && ctrl)) {
                                root.controlCenterModel.closeUtility();
                                event.accepted = true;
                            } else if (event.key === Qt.Key_Tab) {
                                // Cycle category
                                const cats = root.availableCategories;
                                const curIdx = cats.indexOf(root.activeCategory);
                                const nextIdx = (curIdx + 1) % cats.length;
                                root.activeCategory = cats[nextIdx];
                                root.selectedIndex = 0;
                                event.accepted = true;
                            }
                        }
                    }

                    UiText {
                        visible: root.query.length > 0
                        text: "Esc: Clear"
                        color: Theme.textMuted
                        font.pixelSize: Theme.tinyFontSize
                    }
                }

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLg + 20
                    verticalAlignment: Text.AlignVCenter
                    visible: searchTextInput.text.length === 0
                    text: "Search keybindings (e.g. gaps, layout, terminal, Super+Return)..."
                    color: Theme.placeholder
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.inputFontSize
                }
            }

            // Category Filter Pills Bar
            Flickable {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                contentWidth: categoryRow.width
                clip: true

                Row {
                    id: categoryRow
                    spacing: Theme.spacingSm

                    Repeater {
                        model: root.availableCategories

                        delegate: Rectangle {
                            required property string modelData
                            readonly property bool isSelected: root.activeCategory === modelData

                            height: 28
                            width: catText.implicitWidth + Theme.spacingLg * 2
                            color: isSelected ? Theme.surfaceActive : Theme.controlNormalFill
                            border.color: isSelected ? Theme.accent : Theme.controlNormalBorder
                            border.width: 1
                            radius: 0

                            UiText {
                                id: catText
                                anchors.centerIn: parent
                                text: parent.modelData
                                color: parent.isSelected ? Theme.accent : Theme.textMuted
                                font.pixelSize: Theme.smallFontSize
                                font.bold: parent.isSelected
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.activeCategory = parent.modelData;
                                    root.selectedIndex = 0;
                                    root.focusSearch();
                                }
                            }
                        }
                    }
                }
            }

            // Main Results List
            ListView {
                id: resultsList

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: Theme.spacingXs
                model: root.filteredRows

                onModelChanged: {
                    if (root.filteredRows.length > 0) {
                        positionViewAtIndex(Math.min(root.selectedIndex, root.filteredRows.length - 1), ListView.Contain);
                    }
                }

                Connections {
                    target: root

                    function onSelectedIndexChanged() {
                        if (root.filteredRows.length > 0) {
                            resultsList.positionViewAtIndex(root.selectedIndex, ListView.Contain);
                        }
                    }
                }

                delegate: Rectangle {
                    id: rowItem

                    required property var modelData
                    required property int index
                    readonly property bool selected: index === root.selectedIndex

                    width: resultsList.width
                    height: 42
                    color: selected ? Theme.surfaceActive : (rowMouse.containsMouse ? Theme.surfaceHover : Theme.surface)
                    border.color: selected ? Theme.accent : Theme.controlNormalBorder
                    border.width: 1
                    radius: 0

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMd
                        anchors.rightMargin: Theme.spacingMd
                        spacing: Theme.spacingMd

                        // Shortcut Key Badge
                        Rectangle {
                            Layout.preferredWidth: 200
                            Layout.preferredHeight: 28
                            color: rowItem.selected ? Theme.controlNormalFill : Theme.bg
                            border.color: rowItem.selected ? Theme.accent : Theme.border
                            border.width: 1
                            radius: 0

                            UiText {
                                anchors.centerIn: parent
                                text: rowItem.modelData.keys || ""
                                color: rowItem.selected ? Theme.accent : Theme.textStrong
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.smallFontSize
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }

                        // Description
                        UiText {
                            Layout.fillWidth: true
                            text: rowItem.modelData.description || ""
                            color: rowItem.selected ? Theme.textStrong : Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBodySize
                            font.bold: rowItem.selected
                            elide: Text.ElideRight
                        }

                        // Category Tag
                        UiText {
                            text: "[ " + (rowItem.modelData.category || "General") + " ]"
                            color: rowItem.selected ? Theme.accentSecondary : Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.tinyFontSize
                        }

                        // Execute Indicator
                        Rectangle {
                            visible: rowItem.selected
                            Layout.preferredHeight: 24
                            Layout.preferredWidth: runText.implicitWidth + Theme.spacingMd * 2
                            color: Theme.accent
                            radius: 0

                            UiText {
                                id: runText
                                anchors.centerIn: parent
                                text: "↵ Execute"
                                color: Theme.accentText
                                font.pixelSize: Theme.tinyFontSize
                                font.bold: true
                            }
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: root.selectedIndex = rowItem.index
                        onClicked: {
                            root.selectedIndex = rowItem.index;
                            root.executeSelected();
                        }
                    }
                }
            }
        }
    }
}
