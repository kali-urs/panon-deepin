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
    memmove(m_image.scanLine(0), m_image.scanLine(0) + 4, (w - 1) * 4 * h);
    for (int y = 0; y < h; ++y)
        reinterpret_cast<uint32_t *>(m_image.scanLine(y))[w - 1] = 0;
}

void SpectrogramEffect::addColumn(const QVector<double> &data, int xOffset)
{
    if (m_image.isNull()) return;
    int h = m_image.height();
    int w = m_image.width();
    int halfW = w / 2;
    for (int y = 0; y < h; ++y) {
        int specIdx = (y * data.size()) / h;
        double val = std::clamp(data[specIdx], 0.0, 1.0);
        double dbVal = std::log10(1.0 + val * 100.0) / 2.0;
        QColor color = specColor(dbVal);
        int px = xOffset;
        m_image.setPixelColor(px, h - 1 - y, color);
    }
}

void SpectrogramEffect::render(QPainter &p, const QRectF &rect,
                                const QVector<double> &left,
                                const QVector<double> &right,
                                const QVector<double> &,
                                bool vertical)
{
    if (left.isEmpty() || right.isEmpty()) return;

    QMutexLocker lock(&m_mutex);

    int iw, ih;
    if (vertical) {
        iw = static_cast<int>(rect.height());
        ih = static_cast<int>(rect.width());
    } else {
        iw = static_cast<int>(rect.width());
        ih = static_cast<int>(rect.height());
    }

    int imgW = iw * 2;

    if (m_image.isNull() || m_image.width() != imgW || m_image.height() != ih) {
        m_image = QImage(imgW, ih, QImage::Format_ARGB32);
        m_image.fill(Qt::transparent);
        m_colCount = 0;
    }

    scrollImage();
    addColumn(left, 0);
    addColumn(right, iw);
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
