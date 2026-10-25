#include "controller.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QtWebEngineQuick>

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false); // the window is hidden while capturing
    KLocalizedString::setApplicationDomain("aeyes");
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationName(QStringLiteral("aeyes"));
    QGuiApplication::setDesktopFileName(QStringLiteral("org.kde.aeyes"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("aeyes"));
    QGuiApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("org.kde.aeyes"), QIcon::fromTheme(QStringLiteral("camera-photo"))));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    KAboutData about(QStringLiteral("aeyes"), i18n("aeyes"), QStringLiteral(AEYES_VERSION), i18n("Ask an AI about anything on your screen."), KAboutLicense::GPL_V3);
    KAboutData::setApplicationData(about);
    KCrash::initialize();

    KDBusService service(KDBusService::Unique);
    Controller controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextObject(new KLocalizedQmlContext(&engine));
    engine.rootContext()->setContextProperty(QStringLiteral("App"), &controller);
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &engine, [](const QList<QQmlError> &w) {
        for (const QQmlError &e : w)
            qWarning().noquote() << e.toString();
    });
    engine.loadFromModule("org.kde.aeyes.ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        QQmlComponent probe(&engine, "org.kde.aeyes.ui", "Main");
        for (const QQmlError &e : probe.errors())
            fprintf(stderr, "%s\n", qPrintable(e.toString()));
        fprintf(stderr, "Failed to load the user interface.\n");
        return 1;
    }

    QObject::connect(&service, &KDBusService::activateRequested, &controller, [&controller](const QStringList &args, const QString &) {
        controller.handle(args);
    });
    controller.handle(app.arguments());

    // Debug aid: AEYES_CLICK="x,y" clicks into the window after 1.5 s (before any AEYES_GRAB).
    if (const QStringList xy = qEnvironmentVariable("AEYES_CLICK").split(QLatin1Char(',')); xy.size() == 2) {
        QTimer::singleShot(1500, &app, [&engine, xy] {
            if (auto *w = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0))) {
                const QPointF p(xy[0].toDouble(), xy[1].toDouble());
                auto send = [w, p](QEvent::Type type) {
                    QMouseEvent e(type, p, w->mapToGlobal(p), Qt::LeftButton, type == QEvent::MouseButtonPress ? Qt::LeftButton : Qt::NoButton, Qt::NoModifier);
                    QCoreApplication::sendEvent(w, &e);
                };
                QMouseEvent mv(QEvent::MouseMove, p, w->mapToGlobal(p), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                QCoreApplication::sendEvent(w, &mv);
                send(QEvent::MouseButtonPress);
                QTimer::singleShot(120, w, [send] { send(QEvent::MouseButtonRelease); });
            }
        });
    }

    // Debug aid for docs/CI: AEYES_GRAB=file.png saves the window after AEYES_GRAB_DELAY ms and quits.
    if (const QString grab = qEnvironmentVariable("AEYES_GRAB"); !grab.isEmpty()) {
        QTimer::singleShot(qEnvironmentVariableIntValue("AEYES_GRAB_DELAY") ?: 4000, &app, [&engine, grab] {
            if (auto *w = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0)))
                w->grabWindow().save(grab);
            QCoreApplication::quit();
        });
    }
    return app.exec();
}
