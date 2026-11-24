#pragma once
#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QUrl>
#include <QVariantList>

// Which songs were shown to the congregation, and when: the data for the usage report (for example the
// CCLI report). A song counts once per time the output was open; rehearsing with the output closed does not count.
class UsageLog : public QObject {
    Q_OBJECT
public:
    struct Entry { QDateTime when; QString title, ccli, author, service; };

    explicit UsageLog(QObject *parent = nullptr);
    void record(const QString &title, const QString &ccli, const QString &author, const QString &service,
                const QDateTime &when = QDateTime::currentDateTime());

    // period: "month" (this month), "lastmonth", "year" (this year), "all".
    // Returns one row per song: {title, ccli, author, count, last (yyyy-MM-dd)}, the most used first.
    Q_INVOKABLE QVariantList summary(const QString &period) const;
    Q_INVOKABLE int uses(const QString &period) const;                 // total number of times, all songs
    Q_INVOKABLE QString periodText(const QString &period) const;       // "november 2026", "2026", "alles"
    // Writes the summary of the period as a CSV file for a spreadsheet (; separated, UTF-8). False if the file cannot be written.
    Q_INVOKABLE bool exportCsv(const QUrl &file, const QString &period) const;
    Q_INVOKABLE void clear();

    // The same with exact dates (both included); a null date means no limit.
    QVariantList summary(const QDate &from, const QDate &to) const;
    QString csv(const QDate &from, const QDate &to) const;
    static void range(const QString &period, const QDate &today, QDate *from, QDate *to);
    int count() const { return m_entries.size(); }
signals:
    void changed();

private:
    void load();
    void save() const;
    QString path() const;
    QList<Entry> m_entries;
};
