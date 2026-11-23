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
    app.setApplicationVersion(APP_VERSION);
    // "Basic" is the same on every OS, so the UI looks identical on Windows/macOS/Linux.
    QQuickStyle::setStyle("Basic");

    Presenter presenter;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("presenter", &presenter);
    const QStringList cli = app.arguments();
    auto cliValue = [&cli](const QString &key) { int i = cli.indexOf(key); return (i >= 0 && i + 1 < cli.size()) ? cli[i + 1] : QString(); };
    engine.rootContext()->setContextProperty("devTab", cliValue("--tab").isEmpty() ? -1 : cliValue("--tab").toInt());
    engine.rootContext()->setContextProperty("devQuery", cliValue("--query"));
    engine.rootContext()->setContextProperty("devPage", cliValue("--page").isEmpty() ? 0 : cliValue("--page").toInt());
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    const QStringList args = app.arguments();
    // Developer aid: `--import file` adds a file to the media library.
    if (const QString f = cliValue("--import"); !f.isEmpty()) presenter.importMedia(QUrl::fromLocalFile(f));
    // Developer aid: `--background /path/file.mp4` uses a file as default background.
    if (const QString bg = cliValue("--background"); !bg.isEmpty()) presenter.setDefaultBackground(QUrl::fromLocalFile(bg).toString());
    // Developer aid: `--page N` opens a top-level page (0 = Dienst, 2 = Templates, ...), `--template id` activates a template.
    if (const QString t = cliValue("--template"); !t.isEmpty()) presenter.templates()->setActive(t);
    // Developer aid: `--passage "Psalm 23:1-4"` adds a Bible passage and shows it.
    if (const QString ref = cliValue("--passage"); !ref.isEmpty()) presenter.addPassage(ref, 2, true, true);
    // Developer aid: `--demo` starts the service and steps through two slides.
    if (args.contains("--demo")) {
        presenter.startService();
        presenter.nextSlide();
        presenter.nextSlide();
    }
    // Developer aid: `beaconview --screenshot out.png` renders the window to a file and exits.
    if (int i = args.indexOf("--screenshot"); i >= 0 && i + 1 < args.size()) {
        const QString path = args[i + 1];
        QTimer::singleShot(qEnvironmentVariableIntValue("BEACONVIEW_SHOT_DELAY") > 0 ? qEnvironmentVariableIntValue("BEACONVIEW_SHOT_DELAY") : 1500, &app, [&engine, &app, path] {
            auto *w = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
            if (w) w->grabWindow().save(path);
            app.quit();
        });
    }
    return app.exec();
}
