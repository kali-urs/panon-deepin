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
    if (waveform.size() < 2) return;

    double w = rect.width();
    double h = rect.height();
    int n = waveform.size();
    int step = std::max(1, n / 256);

    auto drawWave = [&](double sign, double yOff) {
        QPainterPath path;
        if (vertical) {
            double yStep = h / n;
            for (int i = 0; i < n; i += step) {
                double val = std::clamp(waveform[i], -1.0, 1.0);
                double x = w / 2 + val * w * 0.4 * sign;
                double y = i * yStep;
                if (i == 0) path.moveTo(x, y);
                else path.lineTo(x, y);
            }
        } else {
            double xStep = w / n;
            for (int i = 0; i < n; i += step) {
                double val = std::clamp(waveform[i], -1.0, 1.0);
                double x = i * xStep;
                double y = h / 2 + val * h * 0.4 * sign;
                if (i == 0) path.moveTo(x, y);
                else path.lineTo(x, y);
            }
        }
        QColor c = lerpColor(0.5);
        c.setAlpha(30);
        p.setPen(QPen(c, 12));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
        c.setAlpha(60);
        p.setPen(QPen(c, 6));
        p.drawPath(path);
        c.setAlpha(200);
        p.setPen(QPen(c, 2));
        p.drawPath(path);
    };

    drawWave(-1.0, 0);  // L channel (bottom/left)
    drawWave(1.0, 0);   // R channel (top/right)
}
