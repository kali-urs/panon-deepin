#include "bareffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BarEffect::BarEffect() = default;

void BarEffect::render(QPainter &p, const QRectF &rect,
                        const QVector<double> &left,
                        const QVector<double> &right,
                        const QVector<double> &,
                        bool vertical)
{
    int n = std::min<int>(m_barCount, std::min<int>(left.size(), right.size()));
    if (m_peakHold.size() != n * 2) {
        m_peakHold = QVector<double>(n * 2, 0.0);
    }

    double w = rect.width();
    double h = rect.height();
    double half = vertical ? w * 0.5 : h * 0.5;

    for (int i = 0; i < n; ++i) {
        int idx = (i * left.size()) / n;
        double lVal = std::clamp(left[idx], 0.0, 1.0);
        double rVal = std::clamp(right[idx], 0.0, 1.0);
        if (lVal < 0.01) lVal = 0.0;
        if (rVal < 0.01) rVal = 0.0;

        int lPeak = i;
        int rPeak = n + i;
        m_peakHold[lPeak] = std::max(m_peakHold[lPeak] * m_decayRate, lVal);
        m_peakHold[rPeak] = std::max(m_peakHold[rPeak] * m_decayRate, rVal);

        double t = (double)i / std::max(1, n - 1);
        QColor lColor = lerpColor(t);
        lColor.setAlpha(220);
        QColor rColor = lerpColor(t);
        rColor.setAlpha(220);

        double gap = 0.15;
        double radius = 2.0;

        if (vertical) {
            double unit = h / n;
            double barH = std::max(1.0, unit - unit * gap);
            double barY = i * unit + gap / 2;

            // Left channel from right edge inward
            double lBarW = lVal * half * 0.92;
            QRectF lRect(w - lBarW, barY, lBarW, barH);
            QPainterPath lPath;
            lPath.addRoundedRect(lRect, radius, radius);
            p.fillPath(lPath, lColor);
            if (m_peakHold[lPeak] > 0.01) {
                double pw = m_peakHold[lPeak] * half * 0.92;
                p.fillRect(QRectF(w - pw, barY, pw, 2), QColor(255, 255, 255, 180));
            }

            // Right channel from left edge inward
            double rBarW = rVal * half * 0.92;
            QRectF rRect(0, barY, rBarW, barH);
            QPainterPath rPath;
            rPath.addRoundedRect(rRect, radius, radius);
            p.fillPath(rPath, rColor);
            if (m_peakHold[rPeak] > 0.01) {
                double pw = m_peakHold[rPeak] * half * 0.92;
                p.fillRect(QRectF(0, barY, pw, 2), QColor(255, 255, 255, 180));
            }
        } else {
            double unit = w / n;
            double barW = std::max(1.0, unit - unit * gap);
            double barX = i * unit + gap / 2;

            // Left channel from bottom upward
            double lBarH = lVal * half * 0.92;
            QRectF lRect(barX, h - lBarH, barW, lBarH);
            QPainterPath lPath;
            lPath.addRoundedRect(lRect, radius, radius);
            p.fillPath(lPath, lColor);
            if (m_peakHold[lPeak] > 0.01) {
                double ph = m_peakHold[lPeak] * half * 0.92;
                p.fillRect(QRectF(barX, h - ph, barW, 2), QColor(255, 255, 255, 180));
            }

            // Right channel from top downward
            double rBarH = rVal * half * 0.92;
            QRectF rRect(barX, 0, barW, rBarH);
            QPainterPath rPath;
            rPath.addRoundedRect(rRect, radius, radius);
            p.fillPath(rPath, rColor);
            if (m_peakHold[rPeak] > 0.01) {
                double ph = m_peakHold[rPeak] * half * 0.92;
                p.fillRect(QRectF(barX, 0, barW, 2), QColor(255, 255, 255, 180));
            }
        }
    }

    // Center separator
    if (half > 2) {
        p.setPen(QPen(QColor(255, 255, 255, 25), 1));
        if (vertical)
            p.drawLine(QPointF(half, 0), QPointF(half, h));
        else
            p.drawLine(QPointF(0, half), QPointF(w, half));
    }
}
