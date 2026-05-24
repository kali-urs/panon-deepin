#include "hilleffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

HillEffect::HillEffect() = default;

void HillEffect::render(QPainter &p, const QRectF &rect,
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

    auto gaussian = [](double dist, double sigma) {
        return std::exp(-dist * dist * sigma);
    };

    QPainterPath path;

    if (vertical) {
        double step = h / n;
        path.moveTo(w, 0);
        for (int i = 0; i < n; ++i) {
            double cy = i * step;
            double sum = 0;
            for (int j = 0; j < n; ++j) {
                double amp = std::clamp(spectrum[j], 0.0, 1.0);
                double dist = (double)(i - j) / n * n;
                sum += amp * gaussian(dist, 3.0);
            }
            double x = w - sum * w * 0.85;
            path.lineTo(x, cy);
        }
        path.lineTo(w, h);
        path.closeSubpath();
    } else {
        double step = w / n;
        path.moveTo(0, h);
        for (int i = 0; i < n; ++i) {
            double cx = i * step;
            double sum = 0;
            for (int j = 0; j < n; ++j) {
                double amp = std::clamp(spectrum[j], 0.0, 1.0);
                double dist = (double)(i - j) / n * n;
                sum += amp * gaussian(dist, 3.0);
            }
            double y = h - sum * h * 0.85;
            path.lineTo(cx, y);
        }
        path.lineTo(w, h);
        path.closeSubpath();
    }

    QColor fill = lerpColor(0.4);
    fill.setAlpha(100);
    p.setBrush(fill);
    p.setPen(Qt::NoPen);
    p.drawPath(path);

    QPainterPath linePath = path;
    QColor line = lerpColor(0.7);
    line.setAlpha(200);
    p.setPen(QPen(line, 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(linePath);
}
