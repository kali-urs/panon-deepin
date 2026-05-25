#include "hilleffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

HillEffect::HillEffect() = default;

void HillEffect::render(QPainter &p, const QRectF &rect,
                          const QVector<double> &left,
                          const QVector<double> &,
                          const QVector<double> &,
                          bool vertical)
{
    if (left.isEmpty()) return;

    double w = rect.width();
    double h = rect.height();
    int n = std::min(64, static_cast<int>(left.size()));

    auto gaussian = [](double dist, double sigma) {
        return std::exp(-dist * dist * sigma);
    };

    QPainterPath path;

    if (vertical) {
        double step = h / n;
        path.moveTo(w, 0);
        for (int i = 0; i < n; ++i) {
            double cy = i * step + step / 2;
            double sum = 0;
            for (int j = 0; j < n; ++j) {
                double amp = std::clamp(left[j], 0.0, 1.0);
                double dist = (double)(i - j) / n * n;
                sum += amp * gaussian(dist, 2.0);
            }
            double x = w - std::clamp(sum, 0.0, 1.0) * w * 0.9;
            path.lineTo(x, cy);
        }
        path.lineTo(w, h);
        path.closeSubpath();
    } else {
        double step = w / n;
        path.moveTo(0, h);
        for (int i = 0; i < n; ++i) {
            double cx = i * step + step / 2;
            double sum = 0;
            for (int j = 0; j < n; ++j) {
                double amp = std::clamp(left[j], 0.0, 1.0);
                double dist = (double)(i - j) / n * n;
                sum += amp * gaussian(dist, 2.0);
            }
            double y = h - std::clamp(sum, 0.0, 1.0) * h * 0.9;
            path.lineTo(cx, y);
        }
        path.lineTo(w, h);
        path.closeSubpath();
    }

    QLinearGradient grad;
    if (vertical) {
        grad = QLinearGradient(0, 0, w, 0);
    } else {
        grad = QLinearGradient(0, 0, 0, h);
    }
    QColor g0 = lerpColor(0.0); g0.setAlpha(60);
    QColor g1 = lerpColor(0.4); g1.setAlpha(100);
    QColor g2 = lerpColor(0.7); g2.setAlpha(160);
    grad.setColorAt(0.0, g0);
    grad.setColorAt(0.5, g1);
    grad.setColorAt(1.0, g2);

    QColor fillGlow = lerpColor(0.5);
    fillGlow.setAlpha(20);
    p.setPen(QPen(fillGlow, 12));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    p.setBrush(QBrush(grad));
    p.setPen(Qt::NoPen);
    p.drawPath(path);

    QColor lineColor = lerpColor(0.7);
    lineColor.setAlpha(180);
    p.setPen(QPen(lineColor, 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
}
