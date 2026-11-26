#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include "singleinstance.h"
#include "songstudio.h"

// BeaconView Liederen: a program of its own for the songs. It uses the same song database as BeaconView (same data folder), so the
// two can be open at the same time and follow each other's changes.
int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("BeaconView");      // the same names as BeaconView: that gives the same data folder, and so the same songs.db
    app.setApplicationName("BeaconView");
    app.setApplicationDisplayName("BeaconView Liederen");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/assets/icon/icon.png"));
    QQuickStyle::setStyle("Basic");

    const QStringList args = app.arguments();
    auto value = [&args](const QString &key) { int i = args.indexOf(key); return (i >= 0 && i + 1 < args.size()) ? args[i + 1] : QString(); };
    const bool devRun = args.contains("--screenshot") || QGuiApplication::platformName() == "offscreen";

    SingleInstance single("BeaconViewSongs");     // once: a second start brings the first window to the front
    if (!devRun && !single.isPrimary()) return 0;

    SongStudio studio;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("presenter", &studio);          // the name the song page uses
    engine.rootContext()->setContextProperty("devSong", value("--edit-song").isEmpty() ? -1 : value("--edit-song").toInt());
    engine.load(QUrl("qrc:/qml/SongStudio.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    auto *win = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    if (win) QObject::connect(&single, &SingleInstance::activated, win, [win] {
        win->setWindowStates(win->windowStates() & ~Qt::WindowMinimized);
        win->show(); win->raise(); win->requestActivate();
    });
    if (const QString shot = value("--screenshot"); !shot.isEmpty())
        QTimer::singleShot(1500, &app, [&app, win, shot] { if (win) win->grabWindow().save(shot); app.quit(); });
    return app.exec();
}
