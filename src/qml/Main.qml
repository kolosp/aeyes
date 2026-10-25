import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: "aeyes"
    width: Kirigami.Units.gridUnit * 30
    height: Kirigami.Units.gridUnit * 38
    minimumWidth: Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Units.gridUnit * 20

    // Hidden while Spectacle runs, so the window never ends up in the screenshot.
    visible: App.page !== "none" && !App.capturing
    onClosing: Qt.quit()

    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar

    globalDrawer: Kirigami.GlobalDrawer {
        isMenu: true
        actions: [
            Kirigami.Action {
                text: i18n("New Capture")
                icon.name: "camera-photo-symbolic"
                onTriggered: App.newCapture()
            },
            Kirigami.Action {
                text: i18n("History")
                icon.name: "view-history"
                onTriggered: App.show("history")
            },
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "configure"
                onTriggered: App.show("settings")
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                onTriggered: Qt.quit()
            }
        ]
    }

    Component { id: onboardingPage; OnboardingPage {} }
    Component { id: confirmPage; ConfirmPage {} }
    Component { id: chatPage; ChatPage {} }
    Component { id: historyPage; HistoryPage {} }
    Component { id: settingsPage; SettingsPage {} }

    function showPage() {
        const pages = {
            "onboarding": onboardingPage,
            "confirm": confirmPage,
            "chat": chatPage,
            "history": historyPage,
            "settings": settingsPage
        };
        if (!pages[App.page])
            return;
        // Each page gets a sensible window size (only while the window is a normal, un-maximized one).
        const gu = Kirigami.Units.gridUnit;
        const sizes = {
            "onboarding": [28, 30], "confirm": [30, 28], "chat": [30, 38], "history": [30, 34], "settings": [34, 38]
        };
        if (root.visibility === Window.Windowed) {
            root.width = sizes[App.page][0] * gu;
            root.height = sizes[App.page][1] * gu;
        }
        if (pageStack.depth === 0)
            pageStack.push(pages[App.page]);
        else
            pageStack.replace(pages[App.page]);
    }

    Connections {
        target: App
        function onPageChanged() { root.showPage() }
        function onRaiseRequested() {
            if (root.visible) {
                root.raise();
                root.requestActivate();
            }
        }
    }
    Component.onCompleted: showPage()
}
