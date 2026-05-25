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
    double half = vertical ? w * 0.5 : h * 0.5;
    int n = std::min(128, static_cast<int>(left.size()));

    auto drawFill = [&](const QVector<double> &data, double sign, bool isLeft) {
        QPainterPath path;
        if (vertical) {
            double step = h / n;
            double startX = isLeft ? w : 0;
            path.moveTo(startX, 0);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = isLeft ? w - val * half * 0.92 : val * half * 0.92;
                double y = i * step + step / 2;
                path.lineTo(x, y);
            }
            path.lineTo(startX, h);
            path.closeSubpath();
        } else {
            double step = w / n;
            double startY = isLeft ? h : 0;
            path.moveTo(0, startY);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = i * step + step / 2;
                double y = isLeft ? h - val * half * 0.92 : val * half * 0.92;
                path.lineTo(x, y);
            }
            path.lineTo(w, startY);
            path.closeSubpath();
        }

        QColor g0 = lerpColor(0.0); g0.setAlpha(60);
        QColor g1 = lerpColor(0.5); g1.setAlpha(100);
        QColor g2 = lerpColor(1.0); g2.setAlpha(160);

        QLinearGradient grad;
        if (vertical)
            grad = QLinearGradient(isLeft ? w : 0, 0, isLeft ? w - half : half, 0);
        else
            grad = QLinearGradient(0, isLeft ? h : 0, 0, isLeft ? h - half : half);
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

    drawFill(left, -1.0, true);
    drawFill(right, 1.0, false);

    if (half > 2) {
        p.setPen(QPen(QColor(255, 255, 255, 20), 1));
        if (vertical)
            p.drawLine(QPointF(half, 0), QPointF(half, h));
        else
            p.drawLine(QPointF(0, half), QPointF(w, half));
    }
}
