#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include "presenter.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("BeaconView");
    app.setApplicationName("BeaconView");
    app.setApplicationVersion("0.1.0");
    // "Basic" is the same on every OS, so the UI looks identical on Windows/macOS/Linux.
    QQuickStyle::setStyle("Basic");

    Presenter presenter;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("presenter", &presenter);
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    const QStringList args = app.arguments();
    // Developer aid: `--demo` starts the service and steps through two slides.
    if (args.contains("--demo")) {
        presenter.startService();
        presenter.nextSlide();
        presenter.nextSlide();
    }
    // Developer aid: `beaconview --screenshot out.png` renders the window to a file and exits.
    if (int i = args.indexOf("--screenshot"); i >= 0 && i + 1 < args.size()) {
        const QString path = args[i + 1];
        QTimer::singleShot(1500, &app, [&engine, &app, path] {
            auto *w = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            if (w) w->grabWindow().save(path);
            app.quit();
        });
    }
    return app.exec();
}
