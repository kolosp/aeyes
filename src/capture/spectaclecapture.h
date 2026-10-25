#pragma once
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>

// Runs `spectacle -b` to grab a region (or screen/window) into a temp PNG.
class SpectacleCapture : public QObject
{
    Q_OBJECT
public:
    explicit SpectacleCapture(QObject *parent = nullptr);
    // mode: region | screen | activewindow | window | full
    void start(const QString &mode);
    bool running() const { return m_proc.state() != QProcess::NotRunning; }
    static QStringList arguments(const QString &mode, const QString &outFile);

Q_SIGNALS:
    void captured(const QString &path);
    void cancelled();
    void failed(const QString &message);

private:
    QProcess m_proc;
    QTemporaryDir m_dir;
    QString m_out;
};
