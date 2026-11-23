#pragma once
#include <QList>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Bible text (Statenvertaling) with reference parsing ("Joh 3:16", "1 Kor 13:4-7", "Psalm 23")
// and full-text search. Data is loaded lazily from the bundled JSON resource.
class Bible : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList bookNames READ bookNames CONSTANT)
    Q_PROPERTY(QString translation READ translation CONSTANT)
public:
    struct Passage {
        int book = -1;
        int chapter = 0;
        QList<int> verses;   // sorted, unique
        QString error;       // set when parsing failed
        bool bookFound = false;
        bool ok() const { return error.isEmpty() && book >= 0 && !verses.isEmpty(); }
    };
    struct Slide { QString text, reference; };

    explicit Bible(QObject *parent = nullptr, const QString &resource = QStringLiteral(":/data/bible_svv.json"));

    QStringList bookNames() const;
    QString translation() const;

    Passage parse(const QString &input) const;
    QString label(const Passage &p) const;                       // "Psalm 23:1-3"
    QString verseText(int book, int chapter, int verse) const;
    QList<Slide> slides(const Passage &p, int versesPerSlide, bool numbers) const;

    // QML API
    Q_INVOKABLE int bookCount() const;
    Q_INVOKABLE int chapterCount(int book) const;
    Q_INVOKABLE QString bookName(int book) const;
    Q_INVOKABLE QVariantMap lookup(const QString &reference) const;   // {ok, bookFound, error, label, verses:[{n,text}]}
    Q_INVOKABLE QVariantList search(const QString &query, int limit = 50) const;  // [{ref, text}]

    static QString normalize(const QString &s);

private:
    struct Book { QString name, ref; QStringList aliases; QList<QStringList> chapters; };
    void ensureLoaded() const;
    int findBook(const QString &key) const;
    QString verseRuns(const QList<int> &verses) const;

    QString m_resource;
    mutable bool m_loaded = false;
    mutable QString m_translation;
    mutable QList<Book> m_books;
};
