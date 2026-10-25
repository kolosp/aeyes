import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: page
    title: i18n("Set up aeyes")

    readonly property bool isGemini: App.settings.provider === "gemini"
    readonly property bool canContinue: ack.checked
        && (!isGemini || keyField.text.length > 0 || App.secrets.hasKey("gemini"))
        && (isGemini || (App.settings.openaiBaseUrl.length > 0 && App.settings.openaiModel.length > 0))

    function proceed() {
        if (keyField.text.length > 0)
            App.secrets.setKey(App.settings.provider, keyField.text);
        App.finishOnboarding();
    }

    header: ErrorBar {}

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            level: 2
            text: i18n("Where should your screenshots go?")
            Layout.fillWidth: true
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: i18n("aeyes sends the region you select to an AI service and shows the answer. Nothing is sent until you confirm this page.")
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QQC2.ComboBox {
                Kirigami.FormData.label: i18n("Provider:")
                model: [i18n("Google Gemini"), i18n("OpenAI-compatible (OpenAI, OpenRouter, Ollama, …)")]
                currentIndex: page.isGemini ? 0 : 1
                onActivated: App.settings.provider = currentIndex === 0 ? "gemini" : "openai"
            }

            QQC2.CheckBox {
                visible: page.isGemini
                Kirigami.FormData.label: i18n("Billing:")
                text: i18n("My key belongs to a paid / billing-enabled project")
                checked: App.settings.geminiPaid
                onToggled: App.settings.geminiPaid = checked
            }

            QQC2.TextField {
                visible: !page.isGemini
                Kirigami.FormData.label: i18n("Base URL:")
                Layout.fillWidth: true
                text: App.settings.openaiBaseUrl
                onEditingFinished: App.settings.openaiBaseUrl = text
            }
            QQC2.TextField {
                visible: !page.isGemini
                Kirigami.FormData.label: i18n("Model:")
                Layout.fillWidth: true
                text: App.settings.openaiModel
                placeholderText: "gpt-4o-mini, llava, …"
                onEditingFinished: App.settings.openaiModel = text
            }

            QQC2.TextField {
                id: keyField
                Kirigami.FormData.label: page.isGemini ? i18n("API key:") : i18n("API key (optional):")
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: App.secrets.hasKey(App.settings.provider) ? i18n("Stored in your wallet") : ""
            }
            QQC2.Label {
                visible: page.isGemini
                text: "<a href=\"https://aistudio.google.com/apikey\">" + i18n("Get a Gemini API key") + "</a>"
                onLinkActivated: link => Qt.openUrlExternally(link)
            }
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: App.settings.dataPolicyIsWarning ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
            text: App.settings.dataPolicy
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: page.isGemini && !App.settings.geminiPaid
            wrapMode: Text.Wrap
            text: i18n("Tip: the free tier is convenient but not private. Don't capture passwords, banking, private messages or work you are not allowed to share. Enabling billing on the key, or using a local model, avoids this.")
        }

        QQC2.CheckBox {
            id: ack
            Layout.fillWidth: true
            text: i18n("I understand where my screenshots are sent and how they may be used.")
        }

        QQC2.Button {
            text: i18n("Continue")
            icon.name: "go-next"
            highlighted: true
            enabled: page.canContinue
            onClicked: page.proceed()
        }
    }
}
