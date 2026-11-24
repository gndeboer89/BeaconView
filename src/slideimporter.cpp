#include "slideimporter.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPdfDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QMetaObject>
#include <QPointer>
#include <algorithm>

namespace {
const QStringList kPictureExt = {"png", "jpg", "jpeg", "webp", "bmp", "gif"};
const QStringList kPresentationExt = {"pptx", "ppt", "pps", "ppsx", "odp"};

QString ext(const QString &path) { return QFileInfo(path).suffix().toLower(); }

QString safeName(const QString &s) {
    QString n = s;
    n.replace(QRegularExpression("[^A-Za-z0-9._-]+"), "_");
    return n.left(40);
}

// Renders every page of a PDF to PNG files that fit in 1920x1080 (the page shape is kept).
bool renderPdf(const QString &pdf, const QString &outDir, int &counter, QStringList &images, QString &error,
               const SlideImporter::Progress &progress, int totalHint) {
    QPdfDocument doc;
    const QPdfDocument::Error e = doc.load(pdf);
    if (e != QPdfDocument::Error::None || doc.pageCount() <= 0) {
        error = e == QPdfDocument::Error::IncorrectPassword || e == QPdfDocument::Error::UnsupportedSecurityScheme
                    ? QStringLiteral("Dit PDF-bestand is beveiligd met een wachtwoord.")
                    : QStringLiteral("Dit PDF-bestand kan niet worden gelezen.");
        return false;
    }
    for (int p = 0; p < doc.pageCount(); ++p) {
        const QSizeF pt = doc.pagePointSize(p);
        if (pt.width() <= 0 || pt.height() <= 0) continue;
        const double scale = std::min(1920.0 / pt.width(), 1080.0 / pt.height());
        const QSize px(std::max(1, int(pt.width() * scale + 0.5)), std::max(1, int(pt.height() * scale + 0.5)));
        const QImage page = doc.render(p, px);
        if (page.isNull()) continue;
        QImage flat(px, QImage::Format_RGB32);        // PDF pages are transparent: put them on white
        flat.fill(Qt::white);
        QPainter(&flat).drawImage(0, 0, page);
        const QString file = QStringLiteral("%1/slide-%2.png").arg(outDir).arg(++counter, 3, 10, QChar('0'));
        if (!flat.save(file, "PNG")) { error = QStringLiteral("Kon de slides niet opslaan (schijf vol?)."); return false; }
        images << QUrl::fromLocalFile(file).toString();
        if (progress) progress(counter, totalHint > 0 ? totalHint : doc.pageCount(), QStringLiteral("Slide %1 verwerken…").arg(counter));
    }
    return true;
}

// Converts a presentation to a PDF. Returns the PDF path or an empty string (error set).
QString presentationToPdf(const QString &file, const QString &workDir, QString &error) {
    const QString out = workDir + "/" + QFileInfo(file).completeBaseName() + ".pdf";

    // 1) LibreOffice (free, all platforms)
    const QString soffice = SlideImporter::findLibreOffice();
    if (!soffice.isEmpty()) {
        QProcess p;
        const QString profile = QUrl::fromLocalFile(workDir + "/lo-profile").toString();   // private profile: works while LibreOffice is open
        p.start(soffice, {"-env:UserInstallation=" + profile, "--headless", "--norestore", "--convert-to", "pdf", "--outdir", workDir, file});
        if (p.waitForFinished(240000) && QFileInfo::exists(out)) return out;
        p.kill();
    }

#ifdef Q_OS_WIN
    // 2) PowerPoint itself, through COM (Windows)
    {
        auto q = [](QString s) { return s.replace("'", "''"); };
        const QString script = QStringLiteral(
            "$ErrorActionPreference='Stop'; $ppt = New-Object -ComObject PowerPoint.Application; "
            "try { $p = $ppt.Presentations.Open('%1', $true, $false, $false); $p.SaveAs('%2', 32); $p.Close() } finally { $ppt.Quit() }")
            .arg(q(QDir::toNativeSeparators(file)), q(QDir::toNativeSeparators(out)));
        QProcess p;
        p.start("powershell", {"-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-Command", script});
        if (p.waitForFinished(240000) && QFileInfo::exists(out)) return out;
        p.kill();
    }
#endif
#ifdef Q_OS_MACOS
    // 2) Microsoft PowerPoint through AppleScript (macOS)
    if (QFileInfo::exists("/Applications/Microsoft PowerPoint.app")) {
        auto q = [](QString s) { return s.replace("\\", "\\\\").replace("\"", "\\\""); };
        const QString script = QStringLiteral(
            "tell application \"Microsoft PowerPoint\"\n open POSIX file \"%1\"\n"
            " save active presentation in POSIX file \"%2\" as save as PDF\n close active presentation saving no\nend tell")
            .arg(q(file), q(out));
        QProcess p;
        p.start("osascript", {"-e", script});
        if (p.waitForFinished(240000) && QFileInfo::exists(out)) return out;
        p.kill();
    }
#endif

    error = QStringLiteral("Deze presentatie kan op deze computer niet worden omgezet. Installeer LibreOffice (gratis), "
                           "of kies in PowerPoint Bestand → Opslaan als → PDF en importeer dat PDF-bestand.");
    return {};
}
} // namespace

SlideImporter::SlideImporter(const QString &baseDir, QObject *parent) : QObject(parent), m_baseDir(baseDir) {
    if (m_baseDir.isEmpty()) m_baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/presentations";
}

SlideImporter::~SlideImporter() {
    if (m_worker.joinable()) m_worker.join();
}

QString SlideImporter::findLibreOffice() {
    QStringList candidates = {"soffice", "libreoffice"};
#ifdef Q_OS_WIN
    candidates << "C:/Program Files/LibreOffice/program/soffice.exe" << "C:/Program Files (x86)/LibreOffice/program/soffice.exe";
#endif
#ifdef Q_OS_MACOS
    candidates << "/Applications/LibreOffice.app/Contents/MacOS/soffice";
#endif
    for (const QString &c : candidates) {
        const QString found = QFileInfo(c).isAbsolute() ? (QFileInfo::exists(c) ? c : QString()) : QStandardPaths::findExecutable(c);
        if (!found.isEmpty()) return found;
    }
    return {};
}

QString SlideImporter::supportedFilter() {
    return "Presentaties en afbeeldingen (*.pdf *.pptx *.ppt *.pps *.ppsx *.odp *.png *.jpg *.jpeg *.webp *.bmp *.gif)";
}

SlideImporter::Result SlideImporter::importSync(const QList<QUrl> &files, const QString &outBaseDir, const Progress &progress) {
    Result r;
    if (files.isEmpty()) { r.error = QStringLiteral("Geen bestand gekozen."); return r; }

    const QString first = files.first().toLocalFile();
    r.title = QFileInfo(first).completeBaseName();
    const QString outDir = QStringLiteral("%1/%2-%3").arg(outBaseDir).arg(QDateTime::currentMSecsSinceEpoch()).arg(safeName(r.title));
    if (!QDir().mkpath(outDir)) { r.error = QStringLiteral("Kon de map voor de slides niet maken."); return r; }

    int counter = 0;
    for (const QUrl &url : files) {
        const QString path = url.toLocalFile();
        const QString e = ext(path);
        if (!QFileInfo::exists(path)) { r.error = QStringLiteral("Bestand niet gevonden: %1").arg(QFileInfo(path).fileName()); break; }

        if (kPictureExt.contains(e)) {
            const QString dst = QStringLiteral("%1/slide-%2.%3").arg(outDir).arg(++counter, 3, 10, QChar('0')).arg(e);
            if (!QImageReader(path).canRead() || !QFile::copy(path, dst)) { r.error = QStringLiteral("Kan %1 niet lezen als afbeelding.").arg(QFileInfo(path).fileName()); break; }
            r.images << QUrl::fromLocalFile(dst).toString();
            if (progress) progress(counter, files.size(), QStringLiteral("Afbeelding %1 kopiëren…").arg(counter));
        } else if (e == "pdf") {
            if (progress) progress(counter, 0, QStringLiteral("PDF verwerken…"));
            if (!renderPdf(path, outDir, counter, r.images, r.error, progress, 0)) break;
        } else if (kPresentationExt.contains(e)) {
            if (progress) progress(counter, 0, QStringLiteral("Presentatie omzetten (dit kan even duren)…"));
            QTemporaryDir work;
            QString err;
            const QString pdf = work.isValid() ? presentationToPdf(path, work.path(), err) : QString();
            if (pdf.isEmpty()) { r.error = err.isEmpty() ? QStringLiteral("Kon geen tijdelijke map maken.") : err; break; }
            if (!renderPdf(pdf, outDir, counter, r.images, r.error, progress, 0)) break;
        } else {
            r.error = QStringLiteral("Dit bestandstype wordt niet ondersteund: .%1").arg(e);
            break;
        }
    }
    if (r.images.isEmpty() && r.error.isEmpty()) r.error = QStringLiteral("Er zijn geen slides gevonden.");
    if (!r.ok()) { r.images.clear(); QDir(outDir).removeRecursively(); }   // do not leave half-finished imports behind
    else if (files.size() > 1) r.title = QStringLiteral("%1 (+%2)").arg(r.title).arg(files.size() - 1);
    return r;
}

void SlideImporter::importFiles(const QString &tag, const QList<QUrl> &files) {
    if (m_busy) { emit failed(tag, QStringLiteral("Er loopt al een import.")); return; }
    if (m_worker.joinable()) m_worker.join();
    m_busy = true; m_done = 0; m_total = 0; m_status = QStringLiteral("Bezig…");
    emit busyChanged(); emit statusChanged();

    QPointer<SlideImporter> self(this);
    const QString base = m_baseDir;
    m_worker = std::thread([self, tag, files, base] {
        auto progress = [self](int done, int total, const QString &msg) {
            if (!self) return;
            QMetaObject::invokeMethod(self, [self, done, total, msg] {
                if (!self) return;
                self->m_done = done; self->m_total = total; self->m_status = msg;
                emit self->statusChanged();
            }, Qt::QueuedConnection);
        };
        const Result r = importSync(files, base, progress);
        if (!self) return;
        QMetaObject::invokeMethod(self, [self, tag, r] {
            if (!self) return;
            self->m_busy = false;
            emit self->busyChanged();
            if (r.ok()) emit self->finished(tag, r.images, r.title);
            else emit self->failed(tag, r.error);
        }, Qt::QueuedConnection);
    });
}
