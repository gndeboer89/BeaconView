#pragma once
#include <QObject>
#include <QUrl>

// The church's own logo: shown full screen by the "Logo" button and, optionally, as a small
// watermark in a corner of every slide. Exposed to QML as `presenter.branding`.
class Branding : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString source READ source NOTIFY changed)                   // file URL, empty = built-in cross
    Q_PROPERTY(QString bgColor READ bgColor WRITE setBgColor NOTIFY changed)
    Q_PROPERTY(double scale READ scale WRITE setScale NOTIFY changed)       // logo size on the logo screen (0.1..1)
    Q_PROPERTY(bool watermark READ watermark WRITE setWatermark NOTIFY changed)
    Q_PROPERTY(int corner READ corner WRITE setCorner NOTIFY changed)       // 0 TL, 1 TR, 2 BL, 3 BR
    Q_PROPERTY(double watermarkSize READ watermarkSize WRITE setWatermarkSize NOTIFY changed)  // fraction of slide width
public:
    explicit Branding(QObject *parent = nullptr);

    QString source() const { return m_source; }
    QString bgColor() const { return m_bg; }
    void setBgColor(const QString &c);
    double scale() const { return m_scale; }
    void setScale(double s);
    bool watermark() const { return m_watermark; }
    void setWatermark(bool b);
    int corner() const { return m_corner; }
    void setCorner(int c);
    double watermarkSize() const { return m_wmSize; }
    void setWatermarkSize(double s);

    // Copies the image into the app data folder. Returns false if it is not a readable image.
    Q_INVOKABLE bool chooseLogo(const QUrl &file);
    Q_INVOKABLE void clearLogo();

signals:
    void changed();

private:
    QString dir() const;
    void removeStoredLogos();

    QString m_source, m_bg = QStringLiteral("#101322");
    double m_scale = 0.6, m_wmSize = 0.12;
    bool m_watermark = false;
    int m_corner = 3;
};
