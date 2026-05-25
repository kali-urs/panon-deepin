#include "waveeffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

WaveEffect::WaveEffect() = default;

void WaveEffect::render(QPainter &p, const QRectF &rect,
                         const QVector<double> &,
                         const QVector<double> &,
                         const QVector<double> &waveform,
                         bool vertical)
{
    const QVector<double> &data = waveform;
    if (data.size() < 2) return;

    double w = rect.width();
    double h = rect.height();
    int n = data.size();
    int step = std::max(1, n / 256);

    QPainterPath path;
    double cx = 0, cy = 0;

    if (vertical) {
        double yStep = h / n;
        for (int i = 0; i < n; i += step) {
            double val = std::clamp(data[i], -1.0, 1.0);
            double x = w / 2 + val * w * 0.45;
            double y = i * yStep;
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
            if (i == n / 2) { cx = x; cy = y; }
        }
    } else {
        double xStep = w / n;
        for (int i = 0; i < n; i += step) {
            double val = std::clamp(data[i], -1.0, 1.0);
            double x = i * xStep;
            double y = h / 2 + val * h * 0.45;
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
            if (i == n / 2) { cx = x; cy = y; }
        }
    }

    QColor glowColor = lerpColor(0.5);

    QColor outerGlow = glowColor;
    outerGlow.setAlpha(30);
    p.setPen(QPen(outerGlow, 12));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    QColor midGlow = glowColor;
    midGlow.setAlpha(60);
    p.setPen(QPen(midGlow, 6));
    p.drawPath(path);

    QColor lineColor = glowColor;
    lineColor.setAlpha(220);
    p.setPen(QPen(lineColor, 2.5));
    p.drawPath(path);
}
