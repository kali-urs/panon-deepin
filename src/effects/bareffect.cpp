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
    double gap = 0.15;
    double radius = 2.0;

    if (vertical) {
        double half = h * 0.5;
        double unitH = half / n;

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
            QColor color = lerpColor(t);
            color.setAlpha(220);

            double barH = std::max(1.0, unitH - unitH * gap);
            double barY = i * unitH + gap / 2;

            // Left channel on top half, grow rightward from left edge
            double lBarW = lVal * w * 0.92;
            QRectF lRect(0, barY, lBarW, barH);
            QPainterPath lPath;
            lPath.addRoundedRect(lRect, radius, radius);
            p.fillPath(lPath, color);
            if (m_peakHold[lPeak] > 0.01) {
                double pw = m_peakHold[lPeak] * w * 0.92;
                p.fillRect(QRectF(pw, barY, 2, barH), QColor(255, 255, 255, 180));
            }

            // Right channel on bottom half (mirrored), grow rightward from left edge
            double rBarW = rVal * w * 0.92;
            double rBarY = h - (i * unitH + gap / 2) - barH;
            QRectF rRect(0, rBarY, rBarW, barH);
            QPainterPath rPath;
            rPath.addRoundedRect(rRect, radius, radius);
            p.fillPath(rPath, color);
            if (m_peakHold[rPeak] > 0.01) {
                double pw = m_peakHold[rPeak] * w * 0.92;
                p.fillRect(QRectF(pw, rBarY, 2, barH), QColor(255, 255, 255, 180));
            }
        }

        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 25), 1));
            p.drawLine(QPointF(0, half), QPointF(w, half));
        }
    } else {
        double half = w * 0.5;
        double unitW = half / n;

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
            QColor color = lerpColor(t);
            color.setAlpha(220);

            double barW = std::max(1.0, unitW - unitW * gap);
            double barX = i * unitW + gap / 2;

            // Left channel on left half, grow upward from bottom
            double lBarH = lVal * h * 0.92;
            QRectF lRect(barX, h - lBarH, barW, lBarH);
            QPainterPath lPath;
            lPath.addRoundedRect(lRect, radius, radius);
            p.fillPath(lPath, color);
            if (m_peakHold[lPeak] > 0.01) {
                double ph = m_peakHold[lPeak] * h * 0.92;
                p.fillRect(QRectF(barX, h - ph, barW, 2), QColor(255, 255, 255, 180));
            }

            // Right channel on right half (mirrored), grow upward from bottom
            double rBarX = w - (i * unitW + gap / 2) - barW;
            double rBarH = rVal * h * 0.92;
            QRectF rRect(rBarX, h - rBarH, barW, rBarH);
            QPainterPath rPath;
            rPath.addRoundedRect(rRect, radius, radius);
            p.fillPath(rPath, color);
            if (m_peakHold[rPeak] > 0.01) {
                double ph = m_peakHold[rPeak] * h * 0.92;
                p.fillRect(QRectF(rBarX, h - ph, barW, 2), QColor(255, 255, 255, 180));
            }
        }

        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 25), 1));
            p.drawLine(QPointF(half, 0), QPointF(half, h));
        }
    }
}
