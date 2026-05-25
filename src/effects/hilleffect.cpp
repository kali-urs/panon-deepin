#include "hilleffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

HillEffect::HillEffect() = default;

void HillEffect::render(QPainter &p, const QRectF &rect,
                          const QVector<double> &left,
                          const QVector<double> &right,
                          const QVector<double> &,
                          bool vertical)
{
    if (left.isEmpty() || right.isEmpty()) return;

    double w = rect.width();
    double h = rect.height();
    int n = std::min(64, static_cast<int>(left.size()));

    auto drawHill = [&](const QVector<double> &data, bool isLeft, bool vertical) {
        QPainterPath path;
        double half = vertical ? h * 0.5 : w * 0.5;

        if (vertical) {
            double step = half / n;
            double startY = isLeft ? 0 : h;
            path.moveTo(0, startY);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = val * w * 0.9;
                double y = isLeft ? i * step + step / 2 : h - (i * step + step / 2);
                path.lineTo(x, y);
            }
            path.lineTo(0, isLeft ? half : h - half);
            path.closeSubpath();
        } else {
            double step = half / n;
            double startX = isLeft ? 0 : w;
            path.moveTo(startX, h);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = isLeft ? i * step + step / 2 : w - (i * step + step / 2);
                double y = h - val * h * 0.9;
                path.lineTo(x, y);
            }
            path.lineTo(isLeft ? half : w - half, h);
            path.closeSubpath();
        }

        QColor g0 = lerpColor(0.0); g0.setAlpha(50);
        QColor g1 = lerpColor(0.5); g1.setAlpha(100);
        QColor g2 = lerpColor(1.0); g2.setAlpha(170);

        QLinearGradient grad;
        if (vertical)
            grad = QLinearGradient(0, isLeft ? 0 : h, w * 0.3, isLeft ? half : h - half);
        else
            grad = QLinearGradient(isLeft ? 0 : w, h, isLeft ? half : w - half, h * 0.7);
        grad.setColorAt(0.0, g0);
        grad.setColorAt(0.5, g1);
        grad.setColorAt(1.0, g2);

        QColor glow = lerpColor(0.5);
        glow.setAlpha(15);
        p.setPen(QPen(glow, 10));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        p.setBrush(QBrush(grad));
        p.setPen(Qt::NoPen);
        p.drawPath(path);
    };

    if (vertical) {
        drawHill(left, true, true);
        drawHill(right, false, true);
        double half = h * 0.5;
        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 15), 1));
            p.drawLine(QPointF(0, half), QPointF(w, half));
        }
    } else {
        drawHill(left, true, false);
        drawHill(right, false, false);
        double half = w * 0.5;
        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 15), 1));
            p.drawLine(QPointF(half, 0), QPointF(half, h));
        }
    }
}
