import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: page
    title: i18n("Send this to %1?", App.settings.activeModel)

    Component.onCompleted: prompt.forceActiveFocus()
    Shortcut { sequence: "Escape"; onActivated: App.cancelConfirm() }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        ErrorBar { Layout.fillWidth: true }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: App.settings.dataPolicyIsWarning ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
            text: App.settings.dataPolicy
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: App.pendingImage ? "file://" + App.pendingImage : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: false
        }

        QQC2.TextField {
            id: prompt
            Layout.fillWidth: true
            placeholderText: i18n("What's confusing? (optional)")
            onAccepted: App.confirmSend(text)
        }

        RowLayout {
            Layout.fillWidth: true
            QQC2.CheckBox {
                text: i18n("Ask before sending")
                checked: App.settings.confirmBeforeSend
                onToggled: App.settings.confirmBeforeSend = checked
            }
            Item { Layout.fillWidth: true }
            QQC2.Button {
                text: i18n("Cancel")
                icon.name: "dialog-cancel"
                onClicked: App.cancelConfirm()
            }
            QQC2.Button {
                text: i18n("Retake")
                icon.name: "view-refresh"
                onClicked: App.retake()
            }
            QQC2.Button {
                text: i18n("Send")
                icon.name: "document-send"
                highlighted: true
                onClicked: App.confirmSend(prompt.text)
            }
        }
    }
}
