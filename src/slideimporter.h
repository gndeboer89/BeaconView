#pragma once
#include <QList>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <functional>
#include <thread>

// Turns PDF files, presentations (PowerPoint, OpenDocument) and pictures into image slides.
//  - PDF: rendered natively with Qt PDF.
//  - PowerPoint/OpenDocument: first converted to PDF with LibreOffice (if installed), or with PowerPoint itself
//    on Windows/macOS, then rendered like a PDF. Without any of those the user gets a clear hint.
//  - Pictures (png/jpg/webp/bmp/gif): copied as they are.
// Slides are stored under the app data folder, so the original files are no longer needed.
// Exposed to QML as `presenter.importer`.
class SlideImporter : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int done READ done NOTIFY statusChanged)
    Q_PROPERTY(int total READ total NOTIFY statusChanged)
public:
    struct Result {
        QStringList images;   // file URLs, in order
        QString title;
        QString error;        // set when nothing could be imported
        bool ok() const { return error.isEmpty() && !images.isEmpty(); }
    };
    using Progress = std::function<void(int done, int total, const QString &message)>;

    explicit SlideImporter(const QString &baseDir = QString(), QObject *parent = nullptr);
    ~SlideImporter() override;

    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    int done() const { return m_done; }
    int total() const { return m_total; }

    // Runs in the background; `tag` comes back in the signals so several callers can share the importer.
    Q_INVOKABLE void importFiles(const QString &tag, const QList<QUrl> &files);

    // The same work, synchronously (used by the background thread and by tests).
    static Result importSync(const QList<QUrl> &files, const QString &outBaseDir, const Progress &progress = {});
    static QString findLibreOffice();     // path of soffice, or empty
    Q_INVOKABLE static QString supportedFilter();     // for file dialogs

signals:
    void busyChanged();
    void statusChanged();
    void finished(const QString &tag, const QStringList &images, const QString &title);
    void failed(const QString &tag, const QString &message);

private:
    QString m_baseDir;
    bool m_busy = false;
    QString m_status;
    int m_done = 0, m_total = 0;
    std::thread m_worker;
};
