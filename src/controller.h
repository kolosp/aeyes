#pragma once
#include "capture/spectaclecapture.h"
#include "chat.h"
#include "settings/secretstore.h"
#include "settings/settings.h"
#include "store/sessionmodel.h"

#include <QNetworkAccessManager>
#include <QObject>

// Drives the app: which page is shown, capture -> confirm -> chat flow.
class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString page READ page NOTIFY pageChanged) // none|onboarding|confirm|chat|history|settings
    Q_PROPERTY(bool capturing READ capturing NOTIFY capturingChanged)
    Q_PROPERTY(QString pendingImage READ pendingImage NOTIFY pendingImageChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(Settings *settings READ settings CONSTANT)
    Q_PROPERTY(SecretStore *secrets READ secrets CONSTANT)
    Q_PROPERTY(Chat *chat READ chat CONSTANT)
    Q_PROPERTY(SessionModel *sessions READ sessions CONSTANT)
public:
    explicit Controller(QObject *parent = nullptr);

    QString page() const { return m_page; }
    bool capturing() const { return m_capturing; }
    QString pendingImage() const { return m_pendingImage; }
    QString error() const { return m_error; }
    Settings *settings() { return &m_settings; }
    SecretStore *secrets() { return &m_secrets; }
    Chat *chat() { return &m_chat; }
    SessionModel *sessions() { return &m_sessions; }

    // Entry from the command line (first start or D-Bus re-activation).
    void handle(const QStringList &args);

    Q_INVOKABLE void newCapture();
    Q_INVOKABLE void captureAttachment(); // grab a region to attach to the current chat
    Q_INVOKABLE void confirmSend(const QString &prompt);
    Q_INVOKABLE void retake();
    Q_INVOKABLE void cancelConfirm();
    Q_INVOKABLE void finishOnboarding();
    Q_INVOKABLE void show(const QString &page);
    // Open a saved session in the chat page. One call on purpose: the history delegate that triggers
    // this is destroyed when the model refreshes, so QML must not need to run anything afterwards.
    Q_INVOKABLE void openSession(qint64 id);
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE QString clipboardImage(); // saves to a temp PNG, "" if none
    Q_INVOKABLE void dismissError();
    Q_INVOKABLE void openShortcutSettings();

Q_SIGNALS:
    void pageChanged();
    void capturingChanged();
    void pendingImageChanged();
    void errorChanged();
    void attachmentCaptured(const QString &path);
    void raiseRequested();

private:
    bool ready();
    void showHome();
    void beginCapture(bool forAttachment);
    void setPage(const QString &p);
    void setCapturing(bool c);
    void setError(const QString &e);
    void captured(const QString &path);
    void afterCancel();

    QNetworkAccessManager m_nam;
    Settings m_settings;
    SecretStore m_secrets;
    HistoryStore m_store;
    Chat m_chat;
    SessionModel m_sessions;
    SpectacleCapture m_capture;

    QString m_page = QStringLiteral("none");
    bool m_capturing = false;
    bool m_forAttachment = false;
    bool m_startCaptureAfterOnboarding = false;
    QString m_pendingImage;
    QString m_error;
};
