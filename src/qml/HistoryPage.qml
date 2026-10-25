import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: i18n("History")

    actions: [
        Kirigami.Action {
            text: i18n("New Capture")
            icon.name: "camera-photo-symbolic"
            onTriggered: App.newCapture()
        },
        Kirigami.Action {
            text: i18n("Settings")
            icon.name: "configure"
            onTriggered: App.show("settings")
        }
    ]

    header: ColumnLayout {
        spacing: 0
        ErrorBar { Layout.fillWidth: true }
        Kirigami.SearchField {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
            text: App.sessions.search
            onTextChanged: App.sessions.search = text
        }
    }

    ListView {
        id: list
        model: App.sessions
        clip: true

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: list.count === 0
            icon.name: "view-history"
            text: App.sessions.search ? i18n("No matching sessions") : i18n("No sessions yet")
            explanation: App.sessions.search ? "" : i18n("Press your capture shortcut to ask about something on your screen.")
            helpfulAction: Kirigami.Action {
                visible: !App.sessions.search
                text: i18n("New Capture")
                icon.name: "camera-photo-symbolic"
                onTriggered: App.newCapture()
            }
        }

        delegate: QQC2.ItemDelegate {
            width: ListView.view.width
            onClicked: App.openSession(model.sessionId)
            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                Rectangle {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 6
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 4
                    color: Kirigami.Theme.alternateBackgroundColor
                    radius: Kirigami.Units.cornerRadius
                    clip: true
                    Image {
                        anchors.fill: parent
                        source: model.thumb
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        sourceSize.width: Kirigami.Units.gridUnit * 12
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: model.title
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: model.preview
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.Wrap
                        opacity: 0.8
                    }
                    QQC2.Label {
                        text: model.updated + (model.modelName ? " · " + model.modelName : "")
                        font: Kirigami.Theme.smallFont
                        opacity: 0.6
                    }
                }
                QQC2.ToolButton {
                    icon.name: "edit-delete"
                    display: QQC2.AbstractButton.IconOnly
                    text: i18n("Delete")
                    onClicked: App.sessions.remove(model.sessionId)
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                }
            }
        }
    }
}
