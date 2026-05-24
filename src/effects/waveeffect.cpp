#include "waveeffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

WaveEffect::WaveEffect() = default;

void WaveEffect::render(QPainter &p, const QRectF &rect,
                         const QVector<double> &spectrum,
                         const QVector<double> &waveform,
                         bool vertical)
{
    p.fillRect(rect, QColor(0, 0, 0, 40));

    const QVector<double> &data = waveform.isEmpty() ? spectrum : waveform;
    if (data.size() < 2) {
        p.setPen(QColor(100, 100, 100));
        p.drawText(rect, Qt::AlignCenter, "No audio");
        return;
    }

    double w = rect.width();
    double h = rect.height();
    int n = data.size();

    QPainterPath path;
    double cx = 0, cy = 0;

    if (vertical) {
        double step = h / n;
        for (int i = 0; i < n; ++i) {
            double val = std::clamp(data[i], -1.0, 1.0);
            double x = w / 2 + val * w * 0.4;
            double y = i * step;
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
            if (i == n / 2) { cx = x; cy = y; }
        }
    } else {
        double step = w / n;
        for (int i = 0; i < n; ++i) {
            double val = std::clamp(data[i], -1.0, 1.0);
            double x = i * step;
            double y = h / 2 + val * h * 0.4;
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
            if (i == n / 2) { cx = x; cy = y; }
        }
    }

    QColor color = lerpColor(0.5);
    color.setAlpha(180);
    p.setPen(QPen(color, 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    QColor glow = color;
    glow.setAlpha(80);
    p.setPen(QPen(glow, 4));
    p.drawPath(path);
}
