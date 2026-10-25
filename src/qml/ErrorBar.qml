import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    type: Kirigami.MessageType.Error
    text: App.error
    visible: App.error.length > 0
    showCloseButton: true
    onVisibleChanged: if (!visible) App.dismissError()
}
