import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: i18n("Settings")

    readonly property bool isGemini: App.settings.provider === "gemini"

    header: ErrorBar {}

    Kirigami.FormLayout {
        Kirigami.Separator { Kirigami.FormData.isSection: true; Kirigami.FormData.label: i18n("Shortcut") }
        QQC2.Button {
            text: i18n("Assign Shortcut in System Settings…")
            icon.name: "preferences-desktop-keyboard-shortcut"
            onClicked: App.openShortcutSettings()
        }
        QQC2.Label {
            Layout.maximumWidth: Kirigami.Units.gridUnit * 24
            wrapMode: Text.Wrap
            opacity: 0.7
            text: i18n("Keyboard → Shortcuts → aeyes → Capture and Ask. A Copilot key usually reports as Meta+Shift+F23.")
        }

        Kirigami.Separator { Kirigami.FormData.isSection: true; Kirigami.FormData.label: i18n("Capture") }
        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Capture:")
            readonly property var modes: ["region", "screen", "activewindow", "window", "full"]
            model: [i18n("Select a region"), i18n("Current screen"), i18n("Active window"), i18n("Window under cursor"), i18n("All screens")]
            currentIndex: Math.max(0, modes.indexOf(App.settings.captureMode))
            onActivated: App.settings.captureMode = modes[currentIndex]
        }
        QQC2.CheckBox {
            text: i18n("Ask for confirmation before sending")
            checked: App.settings.confirmBeforeSend
            onToggled: App.settings.confirmBeforeSend = checked
        }

        Kirigami.Separator { Kirigami.FormData.isSection: true; Kirigami.FormData.label: i18n("Provider") }
        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Provider:")
            model: [i18n("Google Gemini"), i18n("OpenAI-compatible")]
            currentIndex: page.isGemini ? 0 : 1
            onActivated: App.settings.provider = currentIndex === 0 ? "gemini" : "openai"
        }
        QQC2.ComboBox {
            visible: page.isGemini
            Kirigami.FormData.label: i18n("Model:")
            editable: true
            model: App.settings.geminiModels
            currentIndex: Math.max(0, find(App.settings.geminiModel))
            editText: App.settings.geminiModel
            onActivated: App.settings.geminiModel = currentText
            onAccepted: App.settings.geminiModel = editText
        }
        QQC2.CheckBox {
            visible: page.isGemini
            text: i18n("My key belongs to a paid / billing-enabled project")
            checked: App.settings.geminiPaid
            onToggled: App.settings.geminiPaid = checked
        }
        QQC2.TextField {
            visible: !page.isGemini
            Kirigami.FormData.label: i18n("Base URL:")
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            text: App.settings.openaiBaseUrl
            onEditingFinished: App.settings.openaiBaseUrl = text
        }
        QQC2.TextField {
            visible: !page.isGemini
            Kirigami.FormData.label: i18n("Model:")
            Layout.preferredWidth: Kirigami.Units.gridUnit * 20
            text: App.settings.openaiModel
            onEditingFinished: App.settings.openaiModel = text
        }
        RowLayout {
            Kirigami.FormData.label: i18n("API key:")
            QQC2.TextField {
                id: keyField
                Layout.preferredWidth: Kirigami.Units.gridUnit * 16
                echoMode: TextInput.Password
                placeholderText: App.secrets.hasKey(App.settings.provider) ? i18n("Stored in your wallet") : i18n("Not set")
            }
            QQC2.Button {
                text: i18n("Save")
                enabled: keyField.text.length > 0
                onClicked: { App.secrets.setKey(App.settings.provider, keyField.text); keyField.text = "" }
            }
            QQC2.Button {
                text: i18n("Remove")
                visible: App.secrets.hasKey(App.settings.provider)
                onClicked: App.secrets.setKey(App.settings.provider, "")
            }
        }
        Kirigami.InlineMessage {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 28
            visible: !App.secrets.walletAvailable
            type: Kirigami.MessageType.Warning
            text: i18n("The wallet is not available, so the key is only kept until aeyes exits. You can also set GEMINI_API_KEY / OPENAI_API_KEY in your environment.")
        }
        Kirigami.InlineMessage {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 28
            visible: true
            type: App.settings.dataPolicyIsWarning ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
            text: App.settings.dataPolicy
        }

        Kirigami.Separator { Kirigami.FormData.isSection: true; Kirigami.FormData.label: i18n("Answers") }
        QQC2.TextField {
            Kirigami.FormData.label: i18n("Answer language:")
            Layout.preferredWidth: Kirigami.Units.gridUnit * 14
            placeholderText: i18n("Same as the screenshot")
            text: App.settings.language
            onEditingFinished: App.settings.language = text
        }
        QQC2.TextArea {
            Kirigami.FormData.label: i18n("System prompt:")
            Layout.preferredWidth: Kirigami.Units.gridUnit * 28
            Layout.preferredHeight: Kirigami.Units.gridUnit * 7
            wrapMode: TextEdit.Wrap
            text: App.settings.systemPrompt
            onEditingFinished: App.settings.systemPrompt = text === App.settings.defaultPrompt() ? "" : text
        }

        Kirigami.Separator { Kirigami.FormData.isSection: true; Kirigami.FormData.label: i18n("History") }
        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Delete sessions older than:")
            from: 0
            to: 3650
            value: App.settings.retentionDays
            textFromValue: v => v === 0 ? i18n("Never") : i18np("%1 day", "%1 days", v)
            valueFromText: t => parseInt(t) || 0
            onValueModified: App.settings.retentionDays = value
        }
    }
}
