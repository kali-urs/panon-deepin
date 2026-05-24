#include "spectrogrameffect.h"
#include <QPainter>
#include <algorithm>
#include <cmath>

SpectrogramEffect::SpectrogramEffect() = default;

void SpectrogramEffect::reset()
{
    QMutexLocker lock(&m_mutex);
    m_image = QImage();
    m_colCount = 0;
}

void SpectrogramEffect::scrollImage()
{
    int newW = m_image.width() + 1;
    if (m_image.isNull()) return;

    QImage scrolled(m_image.width() + 1, m_image.height(), QImage::Format_ARGB32);
    scrolled.fill(Qt::transparent);

    for (int y = 0; y < m_image.height(); ++y) {
        for (int x = 0; x < m_image.width(); ++x) {
            scrolled.setPixelColor(x + 1, y, m_image.pixelColor(x, y));
        }
    }
    m_image = scrolled;
}

void SpectrogramEffect::addColumn(const QVector<double> &spectrum)
{
    if (m_image.isNull()) return;
    int h = m_image.height();

    for (int y = 0; y < h; ++y) {
        int specIdx = (y * spectrum.size()) / h;
        double val = std::clamp(spectrum[specIdx], 0.0, 1.0);
        QColor color = lerpColor(val);
        int alpha = static_cast<int>(val * 255);
        color.setAlpha(std::clamp(alpha, 0, 255));
        m_image.setPixelColor(0, h - 1 - y, color);
    }
}

void SpectrogramEffect::render(QPainter &p, const QRectF &rect,
                                const QVector<double> &spectrum,
                                const QVector<double> &,
                                bool vertical)
{
    p.fillRect(rect, QColor(0, 0, 0, 40));

    if (spectrum.isEmpty()) {
        p.setPen(QColor(100, 100, 100));
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
