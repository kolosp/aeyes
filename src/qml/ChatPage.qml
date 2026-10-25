import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import QtWebEngine
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: page
    title: App.chat.title.length > 0 ? App.chat.title : "aeyes"
    padding: 0

    property var attachments: []

    actions: [
        Kirigami.Action {
            text: i18n("Stop")
            icon.name: "process-stop"
            visible: App.chat.busy
            onTriggered: App.chat.stop()
        },
        Kirigami.Action {
            text: i18n("Regenerate")
            icon.name: "view-refresh"
            visible: !App.chat.busy && App.chat.count > 0
            onTriggered: App.chat.regenerate()
        },
        Kirigami.Action {
            text: i18n("New Capture")
            icon.name: "camera-photo-symbolic"
            onTriggered: App.newCapture()
        },
        Kirigami.Action {
            text: i18n("History")
            icon.name: "view-history"
            onTriggered: App.show("history")
        }
    ]

    function submit() {
        if (App.chat.busy)
            return;
        const t = input.text.trim();
        if (t.length === 0 && attachments.length === 0)
            return;
        App.chat.send(t, attachments);
        input.text = "";
        attachments = [];
    }

    function applyTheme() {
        const T = Kirigami.Theme;
        const mix = (a, b, f) => Kirigami.ColorUtils.linearInterpolation(a, b, f).toString();
        web.runJavaScript("aeyesTheme(" + JSON.stringify({
            bg: T.backgroundColor.toString(),
            fg: T.textColor.toString(),
            accent: T.highlightColor.toString(),
            muted: mix(T.backgroundColor, T.textColor, 0.6),
            border: mix(T.backgroundColor, T.textColor, 0.2),
            alt: T.alternateBackgroundColor.toString(),
            dark: T.backgroundColor.hslLightness < 0.5
        }) + ")");
    }

    function pushTranscript() {
        web.runJavaScript("aeyesSet(" + App.chat.transcriptJson() + ")");
    }

    Connections {
        target: App.chat
        function onMessagesReset() { if (web.ready) page.pushTranscript() }
        function onLastUpdated(textJson, status, error) {
            if (web.ready)
                web.runJavaScript("aeyesLast(" + textJson + "[0]," + JSON.stringify(status) + "," + JSON.stringify(error) + ")");
        }
    }
    Connections {
        target: App
        function onAttachmentCaptured(path) {
            page.attachments = page.attachments.concat([path]);
            input.forceActiveFocus();
        }
    }

    FileDialog {
        id: fileDialog
        title: i18n("Attach images")
        nameFilters: [i18n("Images (*.png *.jpg *.jpeg *.webp *.bmp)")]
        fileMode: FileDialog.OpenFiles
        onAccepted: page.attachments = page.attachments.concat(selectedFiles.map(u => u.toString()))
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        ErrorBar { Layout.fillWidth: true }

        WebEngineView {
            id: web
            property bool ready: false
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Theme goes in the URL so the first frame is already painted in the right colours.
            url: "qrc:/web/index.html?bg=" + encodeURIComponent(Kirigami.Theme.backgroundColor.toString())
                 + "&dark=" + (Kirigami.Theme.backgroundColor.hslLightness < 0.5 ? "1" : "0")
            opacity: ready ? 1 : 0
            backgroundColor: Kirigami.Theme.backgroundColor
            settings.localContentCanAccessRemoteUrls: false
            settings.javascriptCanOpenWindows: false
            settings.errorPageEnabled: false
            onContextMenuRequested: request => request.accepted = true // no browser menu

            onLoadingChanged: loadRequest => {
                if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
                    ready = true;
                    page.applyTheme();
                    page.pushTranscript();
                }
            }
            onNavigationRequested: request => {
                const u = request.url.toString();
                if (u.startsWith("qrc:") || u.startsWith("about:"))
                    return;
                request.reject();
                if (u.startsWith("aeyes://copy/"))
                    App.copyText(App.chat.textAt(parseInt(u.substring(13))));
                else if (u.startsWith("http://") || u.startsWith("https://"))
                    Qt.openUrlExternally(request.url);
            }
            onNewWindowRequested: request => Qt.openUrlExternally(request.requestedUrl)
        }

    }

    footer: ColumnLayout {
        spacing: 0
        Kirigami.Separator { Layout.fillWidth: true }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            Flow {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                visible: page.attachments.length > 0
                Repeater {
                    model: page.attachments
                    delegate: Item {
                        required property int index
                        required property string modelData
                        width: Kirigami.Units.gridUnit * 5
                        height: Kirigami.Units.gridUnit * 4
                        Image {
                            anchors.fill: parent
                            source: modelData.startsWith("file:") ? modelData : "file://" + modelData
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            sourceSize.width: Kirigami.Units.gridUnit * 10
                        }
                        QQC2.ToolButton {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            icon.name: "dialog-close"
                            display: QQC2.AbstractButton.IconOnly
                            onClicked: page.attachments = page.attachments.filter((_, i) => i !== index)
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.ToolButton {
                    icon.name: "mail-attachment"
                    text: i18n("Attach")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: attachMenu.popup()
                    QQC2.ToolTip.text: text
                    QQC2.ToolTip.visible: hovered
                    QQC2.Menu {
                        id: attachMenu
                        QQC2.MenuItem {
                            text: i18n("Choose Files…")
                            icon.name: "document-open"
                            onTriggered: fileDialog.open()
                        }
                        QQC2.MenuItem {
                            text: i18n("Paste Image")
                            icon.name: "edit-paste"
                            onTriggered: {
                                const p = App.clipboardImage();
                                if (p)
                                    page.attachments = page.attachments.concat([p]);
                            }
                        }
                        QQC2.MenuItem {
                            text: i18n("Capture Region…")
                            icon.name: "camera-photo-symbolic"
                            onTriggered: App.captureAttachment()
                        }
                    }
                }

                QQC2.ScrollView {
                    Layout.fillWidth: true
                    Layout.maximumHeight: Kirigami.Units.gridUnit * 8
                    QQC2.TextArea {
                        id: input
                        placeholderText: App.chat.count === 0 ? "" : i18n("Ask a follow-up…")
                        wrapMode: TextEdit.Wrap
                        Keys.onPressed: event => {
                            if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !(event.modifiers & Qt.ShiftModifier)) {
                                page.submit();
                                event.accepted = true;
                            } else if (event.matches(StandardKey.Paste)) {
                                const p = App.clipboardImage();
                                if (p) {
                                    page.attachments = page.attachments.concat([p]);
                                    event.accepted = true;
                                }
                            }
                        }
                    }
                }

                QQC2.ComboBox {
                    id: modelBox
                    editable: true
                    implicitWidth: Kirigami.Units.gridUnit * 12
                    model: App.settings.provider === "gemini" ? App.settings.geminiModels : [App.settings.openaiModel]
                    currentIndex: Math.max(0, find(App.settings.activeModel))
                    editText: App.settings.activeModel
                    onActivated: App.settings.activeModel = currentText
                    onAccepted: App.settings.activeModel = editText
                    QQC2.ToolTip.text: i18n("Model used for the next answer")
                    QQC2.ToolTip.visible: hovered
                }

                QQC2.Button {
                    Layout.preferredHeight: modelBox.implicitHeight
                    icon.name: App.chat.busy ? "process-stop" : "document-send"
                    text: App.chat.busy ? i18n("Stop") : i18n("Send")
                    highlighted: !App.chat.busy
                    onClicked: App.chat.busy ? App.chat.stop() : page.submit()
                }
            }
        }
    }
}
