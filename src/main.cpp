#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include "presenter.h"
#include "videohub.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("BeaconView");
    app.setApplicationName("BeaconView");
    app.setApplicationVersion(APP_VERSION);
    app.setWindowIcon(QIcon(":/assets/icon/icon.png"));   // window icon (taskbar/Dock use the bundled .ico/.icns)
    // "Basic" is the same on every OS, so the UI looks identical on Windows/macOS/Linux.
    QQuickStyle::setStyle("Basic");

    Presenter presenter;
    VideoHub videoHub;      // must outlive the QML engine: components still use it while they are torn down
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("presenter", &presenter);
    engine.rootContext()->setContextProperty("videoHub", &videoHub);
    const QStringList cli = app.arguments();
    auto cliValue = [&cli](const QString &key) { int i = cli.indexOf(key); return (i >= 0 && i + 1 < cli.size()) ? cli[i + 1] : QString(); };
    engine.rootContext()->setContextProperty("devTab", cliValue("--tab").isEmpty() ? -1 : cliValue("--tab").toInt());
    engine.rootContext()->setContextProperty("devQuery", cliValue("--query"));
    engine.rootContext()->setContextProperty("devEdit", cliValue("--edit").isEmpty() ? -1 : cliValue("--edit").toInt());
    engine.rootContext()->setContextProperty("devLeft", cliValue("--left-mode").isEmpty() ? 0 : cliValue("--left-mode").toInt());
    engine.rootContext()->setContextProperty("devPage", cliValue("--page").isEmpty() ? 0 : cliValue("--page").toInt());
    engine.load(QUrl("qrc:/qml/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    const QStringList args = app.arguments();
    // Developer aid: `--import file` adds a file to the media library.
    if (const QString f = cliValue("--import"); !f.isEmpty()) presenter.importMedia(QUrl::fromLocalFile(f));
    // Developer aid: `--background /path/file.mp4` uses a file as default background.
    if (const QString bg = cliValue("--background"); !bg.isEmpty()) presenter.setDefaultBackground(QUrl::fromLocalFile(bg).toString());
    // Developer aid: `--overlay-demo` shows a 5:00 countdown, the clock and a message.
    if (args.contains("--overlay-demo")) {
        auto *o = presenter.overlays();
        o->setTimerDuration(300); o->setTimerShown(true); o->start(); o->setClockShown(true); o->toggleMessage(1);
    }
    // Developer aid: `--item-template 1=builtin:modern` gives part 1 its own template, `--select 1` goes live on it.
    if (const QString spec = cliValue("--item-template"); spec.contains('=')) presenter.setItemTemplate(spec.section('=', 0, 0).toInt(), spec.section('=', 1));
    if (const QString sel = cliValue("--select"); !sel.isEmpty()) presenter.selectItem(sel.toInt());
    // Developer aid: `--kind-template song=builtin:modern` sets the default template for a kind of part.
    if (const QString spec = cliValue("--kind-template"); spec.contains('=')) presenter.templates()->setKindTemplate(spec.section('=', 0, 0), spec.section('=', 1));
    // Developer aid: `--logo file.png [--watermark]` sets the church logo.
    if (const QString f = cliValue("--logo"); !f.isEmpty()) {
        presenter.branding()->chooseLogo(QUrl::fromLocalFile(f));
        if (args.contains("--watermark")) presenter.branding()->setWatermark(true);
    }
    // Developer aids: `--import-slides file` imports a PDF/PowerPoint/picture as a new part and goes live on it,
    // `--add-video file` adds a video part and goes live on it, `--add-loop` adds an (empty) announcement loop.
    if (const QString f = cliValue("--import-slides"); !f.isEmpty()) {
        QObject::connect(presenter.importer(), &SlideImporter::finished, &app, [&presenter] {
            QTimer::singleShot(300, &presenter, [&presenter] { presenter.selectItem(presenter.service()->count() - 1); });
        });
        presenter.importer()->importFiles("new-slides", {QUrl::fromLocalFile(f)});
    }
    if (const QString f = cliValue("--add-video"); !f.isEmpty()) presenter.selectItem(presenter.addVideo(QUrl::fromLocalFile(f)));
    // Developer aid: `--select-after N` selects part N after everything above was added.
    if (const QString sel = cliValue("--select-after"); !sel.isEmpty()) presenter.selectItem(sel.toInt());
    // Developer aid: `--then-text Title` adds a text part after the other adds (to test "continue after the video").
    if (const QString t = cliValue("--then-text"); !t.isEmpty()) presenter.addItem("prayer", t, "", t);
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
