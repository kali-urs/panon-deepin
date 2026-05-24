#include "spectrogrameffect.h"
#include <QPainter>
#include <QColor>
#include <cstring>
#include <algorithm>
#include <cmath>

SpectrogramEffect::SpectrogramEffect() = default;

void SpectrogramEffect::reset()
{
    QMutexLocker lock(&m_mutex);
    m_image = QImage();
    m_colCount = 0;
}

static QColor specColor(double t)
{
    t = std::clamp(t, 0.0, 1.0);
    if (t < 0.25) {
        double u = t / 0.25;
        return QColor(static_cast<int>(u * 20), static_cast<int>(u * 40), static_cast<int>(60 + u * 195));
    }
    if (t < 0.5) {
        double u = (t - 0.25) / 0.25;
        return QColor(static_cast<int>(20 + u * 235), static_cast<int>(40 + u * 160), 255);
    }
    if (t < 0.75) {
        double u = (t - 0.5) / 0.25;
        return QColor(255, static_cast<int>(200 - u * 130), static_cast<int>(255 - u * 200));
    }
    double u = (t - 0.75) / 0.25;
    return QColor(255, static_cast<int>(70 - u * 70), static_cast<int>(55 - u * 55));
}

void SpectrogramEffect::scrollImage()
{
    if (m_image.isNull() || m_image.width() <= 1) return;
    int w = m_image.width();
    int h = m_image.height();
    int bpl = m_image.bytesPerLine();

    for (int y = 0; y < h; ++y) {
        uchar *line = m_image.scanLine(y);
        memmove(line + 4, line, (w - 1) * 4);
        memset(line, 0, 4);
    }
}

void SpectrogramEffect::addColumn(const QVector<double> &spectrum)
{
    if (m_image.isNull()) return;
    int h = m_image.height();

    for (int y = 0; y < h; ++y) {
        int specIdx = (y * spectrum.size()) / h;
        double val = std::clamp(spectrum[specIdx], 0.0, 1.0);
        double dbVal = std::log10(1.0 + val * 100.0) / 2.0;
        QColor color = specColor(dbVal);
        m_image.setPixelColor(0, h - 1 - y, color);
    }
}

void SpectrogramEffect::render(QPainter &p, const QRectF &rect,
                                const QVector<double> &spectrum,
                                const QVector<double> &,
                                bool vertical)
{
    p.fillRect(rect, QColor(13, 13, 26, 200));

    if (spectrum.isEmpty()) {
        p.setPen(QColor(100, 100, 120));
        p.drawText(rect, Qt::AlignCenter, "No audio");
        return;
    }

    QMutexLocker lock(&m_mutex);

    int iw, ih;
    if (vertical) {
        iw = static_cast<int>(rect.height());
        ih = static_cast<int>(rect.width());
    } else {
        iw = static_cast<int>(rect.width());
        ih = static_cast<int>(rect.height());
    }

    if (m_image.isNull() || m_image.width() != iw || m_image.height() != ih) {
        m_image = QImage(iw, ih, QImage::Format_ARGB32);
        m_image.fill(Qt::transparent);
        m_colCount = 0;
    }

    scrollImage();
    addColumn(spectrum);
    m_colCount++;

    if (vertical) {
        QTransform t;
        t.rotate(90);
        QImage rotated = m_image.transformed(t, Qt::SmoothTransformation);
        p.drawImage(QPointF(0, 0), rotated);
    } else {
        p.drawImage(QPointF(0, 0), m_image);
    }
}
