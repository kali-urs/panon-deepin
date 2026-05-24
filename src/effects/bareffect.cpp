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
    p.fillRect(rect, QColor(13, 13, 26, 200));

    if (spectrum.isEmpty()) {
        p.setPen(QColor(100, 100, 120));
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

        if (val < 0.01) val = 0.0;

        m_peakHold[i] = std::max(m_peakHold[i] * m_decayRate, val);

        double t = (double)i / std::max(1, n - 1);
        double radius = 2.0;

        double barW, barH, barX, barY;
        if (vertical) {
            double barH_unit = h / n;
            double gap = barH_unit * 0.15;
            barH = barH_unit - gap;
            barW = val * w * 0.92;
            barX = w - barW;
            barY = i * barH_unit + gap / 2;
        } else {
            double barW_unit = w / n;
            double gap = barW_unit * 0.15;
            barW = std::max(1.0, barW_unit - gap);
            barH = val * h * 0.92;
            barX = i * barW_unit + gap / 2;
            barY = h - barH;
        }

        QColor barColor = lerpColor(t);
        barColor.setAlpha(220);

        QPainterPath barPath;
        if (vertical) {
            barPath.addRoundedRect(QRectF(barX, barY, barW, barH), 0, radius);
        } else {
            barPath.addRoundedRect(QRectF(barX, barY, barW, barH), radius, 0);
        }
        p.fillPath(barPath, barColor);

        QColor glowColor = barColor;
        glowColor.setAlpha(60);
        p.setPen(QPen(glowColor, 4));
        p.setBrush(Qt::NoBrush);
        if (vertical) {
            p.drawRoundedRect(QRectF(barX - 2, barY, barW + 4, barH), 0, radius);
        } else {
            p.drawRoundedRect(QRectF(barX, barY - 2, barW, barH + 4), radius, 0);
        }

        if (m_peakHold[i] > 0.01) {
            double peakX, peakY, peakW, peakH;
            if (vertical) {
                peakW = m_peakHold[i] * w * 0.92;
                peakX = w - peakW;
                peakY = barY;
                peakH = barH;
                QColor peakColor(255, 255, 255, 180);
                p.fillRect(QRectF(peakX, peakY, 2, peakH), peakColor);
                QColor peakGlow(255, 255, 255, 60);
                p.fillRect(QRectF(peakX - 1, peakY, 4, peakH), peakGlow);
            } else {
                peakH = m_peakHold[i] * h * 0.92;
                peakY = h - peakH;
                peakX = barX;
                peakW = barW;
                QColor peakColor(255, 255, 255, 180);
                p.fillRect(QRectF(peakX, peakY, peakW, 2), peakColor);
                QColor peakGlow(255, 255, 255, 60);
                p.fillRect(QRectF(peakX, peakY - 1, peakW, 4), peakGlow);
            }
        }
    }
}
