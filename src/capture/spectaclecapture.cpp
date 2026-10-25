#include "spectaclecapture.h"

#include <KLocalizedString>
#include <QFile>
#include <QFileInfo>

SpectacleCapture::SpectacleCapture(QObject *parent)
    : QObject(parent)
{
    connect(&m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            Q_EMIT failed(i18n("Could not start Spectacle. Is it installed?"));
    });
    connect(&m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
        // Cancelling the selector leaves no file behind.
        if (QFileInfo(m_out).size() > 0)
            Q_EMIT captured(m_out);
        else if (code == 0 || code == 1)
            Q_EMIT cancelled();
        else
            Q_EMIT failed(i18n("Spectacle exited with code %1.", code));
    });
}

QStringList SpectacleCapture::arguments(const QString &mode, const QString &outFile)
{
    QStringList a{QStringLiteral("-b"), QStringLiteral("-n")};
    if (mode == QLatin1String("screen"))
        a << QStringLiteral("-m");
    else if (mode == QLatin1String("activewindow"))
        a << QStringLiteral("-a");
    else if (mode == QLatin1String("window"))
        a << QStringLiteral("-u");
    else if (mode == QLatin1String("full"))
        a << QStringLiteral("-f");
    else
        a << QStringLiteral("-r");
    a << QStringLiteral("-o") << outFile;
    return a;
}

void SpectacleCapture::start(const QString &mode)
{
    if (running())
        return;
    m_out = m_dir.filePath(QStringLiteral("shot-%1.png").arg(QDateTime::currentMSecsSinceEpoch()));
    m_proc.start(QStringLiteral("spectacle"), arguments(mode, m_out));
}
