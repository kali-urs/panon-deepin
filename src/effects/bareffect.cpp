#include "bareffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BarEffect::BarEffect() = default;

void BarEffect::render(QPainter &p, const QRectF &rect,
                        const QVector<double> &spectrum,
                        const QVector<double> &,
                        bool vertical)
{
    if (spectrum.isEmpty()) {
        p.setPen(QColor(100, 100, 100));
        p.drawText(rect, Qt::AlignCenter, "No audio");
        return;
    }

    int n = std::min(m_barCount, static_cast<int>(spectrum.size()));
    if (m_peakHold.size() != n) {
        m_peakHold = QVector<double>(n, 0.0);
    }

    double w = rect.width();
    double h = rect.height();

    for (int i = 0; i < n; ++i) {
        int idx = (i * spectrum.size()) / n;
        double val = std::clamp(spectrum[idx], 0.0, 1.0);

        m_peakHold[i] = std::max(m_peakHold[i] * m_decayRate, val);

        double t = (double)i / std::max(1, n - 1);
        QColor color = lerpColor(t);
        double radius = 2.0;

        if (vertical) {
            double barH = h / n;
            double gap = barH * 0.2;
            double barActualH = barH - gap;
            double barW = val * w * 0.9;
            double x = w - barW;
            double y = i * barH + gap / 2;

            QPainterPath path;
            path.addRoundedRect(QRectF(x, y, barW, barActualH), radius, radius);
            p.fillPath(path, color);

            double peakW = m_peakHold[i] * w * 0.9;
            double px = w - peakW;
            p.fillRect(QRectF(px, y, 2, barActualH), Qt::white);
        } else {
            double barW = w / n;
            double gap = barW * 0.2;
            double barActualW = std::max(1.0, barW - gap);
            double barH = val * h * 0.9;
            double x = i * barW + gap / 2;
            double y = h - barH;

            QPainterPath path;
            path.addRoundedRect(QRectF(x, y, barActualW, barH), radius, radius);
            p.fillPath(path, color);

            double peakH = m_peakHold[i] * h * 0.9;
            double py = h - peakH;
            p.fillRect(QRectF(x, py, barActualW, 2), Qt::white);
        }
    }
}
