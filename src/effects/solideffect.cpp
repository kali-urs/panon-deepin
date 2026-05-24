#include "solideffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

SolidEffect::SolidEffect() = default;

void SolidEffect::render(QPainter &p, const QRectF &rect,
                          const QVector<double> &spectrum,
                          const QVector<double> &,
                          bool vertical)
{
    if (spectrum.isEmpty()) {
        p.setPen(QColor(100, 100, 100));
        p.drawText(rect, Qt::AlignCenter, "No audio");
        return;
    }

    double w = rect.width();
    double h = rect.height();
    int n = spectrum.size();

    QPainterPath path;
    double cx = vertical ? w : w / 2;
    double cy = vertical ? h / 2 : h;

    if (vertical) {
        path.moveTo(cx, cy);
        for (int i = 0; i < n; ++i) {
            double val = std::clamp(spectrum[i], 0.0, 1.0);
            double step = h / n;
            double x = w * (1.0 - val * 0.85);
            double y = i * step + step / 2;
            path.lineTo(x, y);
        }
        double step = h / n;
        path.lineTo(w * 0.15, h);
        path.closeSubpath();
    } else {
        path.moveTo(0, h);
        for (int i = 0; i < n; ++i) {
            double val = std::clamp(spectrum[i], 0.0, 1.0);
            double step = w / n;
            double x = i * step + step / 2;
            double y = h * (1.0 - val * 0.85);
            path.lineTo(x, y);
        }
        path.lineTo(w, h);
        path.closeSubpath();
    }

    QColor fill = lerpColor(0.5);
    fill.setAlpha(160);
    p.setBrush(fill);
    p.setPen(Qt::NoPen);
    p.drawPath(path);

    QColor line = lerpColor(0.7);
    line.setAlpha(220);
    p.setPen(QPen(line, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}
