#include "solideffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

SolidEffect::SolidEffect() = default;

void SolidEffect::render(QPainter &p, const QRectF &rect,
                          const QVector<double> &left,
                          const QVector<double> &right,
                          const QVector<double> &,
                          bool vertical)
{
    if (left.isEmpty() || right.isEmpty()) return;

    double w = rect.width();
    double h = rect.height();
    int n = std::min(128, static_cast<int>(left.size()));

    auto drawFill = [&](const QVector<double> &data, bool isLeft, bool vertical) {
        QPainterPath path;
        double half = vertical ? h * 0.5 : w * 0.5;

        if (vertical) {
            double step = h / n;
            double startY = isLeft ? 0 : h;
            path.moveTo(0, startY);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = val * (w * 0.92);
                double y = isLeft ? i * step + step / 2 : h - (i * step + step / 2);
                path.lineTo(x, y);
            }
            path.lineTo(0, startY + (isLeft ? h * 0.5 : -h * 0.5));
            path.closeSubpath();
        } else {
            double step = w / n;
            double startX = isLeft ? 0 : w;
            path.moveTo(startX, h);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = isLeft ? i * step + step / 2 : w - (i * step + step / 2);
                double y = h - val * (h * 0.92);
                path.lineTo(x, y);
            }
            path.lineTo(startX + (isLeft ? w * 0.5 : -w * 0.5), h);
            path.closeSubpath();
        }

        QColor g0 = lerpColor(0.0); g0.setAlpha(60);
        QColor g1 = lerpColor(0.5); g1.setAlpha(100);
        QColor g2 = lerpColor(1.0); g2.setAlpha(160);

        QLinearGradient grad;
        if (vertical)
            grad = QLinearGradient(0, isLeft ? 0 : h, 0, isLeft ? half : h - half);
        else
            grad = QLinearGradient(isLeft ? 0 : w, h, isLeft ? half : w - half, h);
        grad.setColorAt(0.0, g0);
        grad.setColorAt(0.5, g1);
        grad.setColorAt(1.0, g2);

        QColor glow = lerpColor(0.5);
        glow.setAlpha(20);
        p.setPen(QPen(glow, 8));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        p.setBrush(QBrush(grad));
        p.setPen(Qt::NoPen);
        p.drawPath(path);
    };

    if (vertical) {
        drawFill(left, true, true);
        drawFill(right, false, true);
        double half = h * 0.5;
        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 20), 1));
            p.drawLine(QPointF(0, half), QPointF(w, half));
        }
    } else {
        drawFill(left, true, false);
        drawFill(right, false, false);
        double half = w * 0.5;
        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 20), 1));
            p.drawLine(QPointF(half, 0), QPointF(half, h));
        }
    }
}
