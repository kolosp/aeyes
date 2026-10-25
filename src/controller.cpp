#include "controller.h"

#include <KLocalizedString>
#include <QClipboard>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QImage>
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>

Controller::Controller(QObject *parent)
    : QObject(parent)
    , m_store(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
    , m_chat(&m_settings, &m_secrets, &m_store, &m_nam)
    , m_sessions(&m_store)
{
    m_store.purgeOlderThan(m_settings.retentionDays());
    connect(&m_chat, &Chat::sessionSaved, &m_sessions, &SessionModel::refresh);
    connect(&m_chat, &Chat::sessionChanged, &m_sessions, &SessionModel::refresh);
    connect(&m_capture, &SpectacleCapture::captured, this, &Controller::captured);
    connect(&m_capture, &SpectacleCapture::cancelled, this, &Controller::afterCancel);
    connect(&m_capture, &SpectacleCapture::failed, this, [this](const QString &msg) {
        setError(msg);
        setCapturing(false);
        if (m_page == QLatin1String("none"))
            setPage(QStringLiteral("history")); // something to show the error in
    });
}

void Controller::setPage(const QString &p)
{
    if (p == m_page)
        return;
    m_page = p;
    Q_EMIT pageChanged();
}

void Controller::setCapturing(bool c)
{
    if (c == m_capturing)
        return;
    m_capturing = c;
    Q_EMIT capturingChanged();
}

void Controller::setError(const QString &e)
{
    m_error = e;
    Q_EMIT errorChanged();
}

void Controller::dismissError()
{
    setError({});
}

void Controller::handle(const QStringList &args)
{
    QCommandLineParser p;
    p.addOption({QStringLiteral("history"), i18n("Open the session history.")});
    p.addOption({QStringLiteral("settings"), i18n("Open the settings.")});
    p.addOption({QStringLiteral("capture"), i18n("Capture a screen region and ask about it.")});
    p.parse(args);
    if (p.isSet(QStringLiteral("history")))
        show(QStringLiteral("history"));
    else if (p.isSet(QStringLiteral("settings")))
        show(QStringLiteral("settings"));
    else if (p.isSet(QStringLiteral("capture")))
        newCapture();
    else
        showHome();
    Q_EMIT raiseRequested();
}

// Plain launch (start menu): the history list doubles as the home screen. Never captures.
void Controller::showHome()
{
    if (m_capture.running())
        return;
    if (!ready()) {
        m_startCaptureAfterOnboarding = false;
        setPage(QStringLiteral("onboarding"));
    } else if (m_page == QLatin1String("none") || m_page == QLatin1String("onboarding")) {
        show(QStringLiteral("history"));
    }
}

void Controller::show(const QString &page)
{
    if (page == QLatin1String("history"))
        m_sessions.refresh();
    setPage(page);
}

void Controller::openSession(qint64 id)
{
    setPage(QStringLiteral("chat"));
    m_chat.open(id);
}

// Nothing is ever sent before the user has seen where it goes and has a key (or a local server).
bool Controller::ready()
{
    if (!m_settings.acknowledged())
        return false;
    if (m_settings.provider() == QLatin1String("gemini"))
        return m_secrets.hasKey(QStringLiteral("gemini"));
    return true;
}

void Controller::newCapture()
{
    if (m_capture.running())
        return;
    if (!ready()) {
        m_startCaptureAfterOnboarding = true;
        setPage(QStringLiteral("onboarding"));
        return;
    }
    beginCapture(false);
}

void Controller::captureAttachment()
{
    if (!m_capture.running())
        beginCapture(true);
}

void Controller::finishOnboarding()
{
    m_settings.acknowledge();
    if (!ready())
        return;
    if (m_startCaptureAfterOnboarding) {
        m_startCaptureAfterOnboarding = false;
        beginCapture(false);
    } else {
        setPage(m_chat.count() ? QStringLiteral("chat") : QStringLiteral("history"));
    }
}

void Controller::beginCapture(bool forAttachment)
{
    m_forAttachment = forAttachment;
    setError({});
    setCapturing(true); // hides the window so it isn't in the screenshot
    // Give KWin a moment to actually unmap the window.
    QTimer::singleShot(forAttachment || m_page != QLatin1String("none") ? 250 : 0, this, [this] { m_capture.start(m_settings.captureMode()); });
}

void Controller::captured(const QString &path)
{
    if (m_forAttachment) {
        setCapturing(false);
        Q_EMIT attachmentCaptured(path);
        return;
    }
    m_pendingImage = path;
    Q_EMIT pendingImageChanged();
    if (m_settings.confirmBeforeSend()) {
        setPage(QStringLiteral("confirm"));
        setCapturing(false);
    } else {
        confirmSend({});
    }
}

void Controller::afterCancel()
{
    setCapturing(false);
    // Nothing on screen to return to: just leave.
    if (m_page == QLatin1String("none") || (m_page == QLatin1String("onboarding") && !m_forAttachment))
        QCoreApplication::quit();
}

void Controller::confirmSend(const QString &prompt)
{
    const QString img = m_pendingImage;
    m_pendingImage.clear();
    Q_EMIT pendingImageChanged();
    setPage(QStringLiteral("chat"));
    setCapturing(false);
    m_chat.startNew(img, prompt);
}

void Controller::retake()
{
    beginCapture(false);
}

void Controller::cancelConfirm()
{
    m_pendingImage.clear();
    Q_EMIT pendingImageChanged();
    if (m_chat.count())
        setPage(QStringLiteral("chat"));
    else
        QCoreApplication::quit();
}

void Controller::copyText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

QString Controller::clipboardImage()
{
    const QImage img = QGuiApplication::clipboard()->image();
    if (img.isNull())
        return {};
    QTemporaryFile f(QDir::tempPath() + QStringLiteral("/aeyes-paste-XXXXXX.png"));
    f.setAutoRemove(false);
    if (!f.open())
        return {};
    img.save(&f, "PNG");
    return f.fileName();
}

void Controller::openShortcutSettings()
{
    QProcess::startDetached(QStringLiteral("systemsettings"), {QStringLiteral("kcm_keys")});
}
