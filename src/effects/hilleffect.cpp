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
    double half = vertical ? w * 0.5 : h * 0.5;
    int n = std::min(64, static_cast<int>(left.size()));

    auto drawHill = [&](const QVector<double> &data, bool isLeft) {
        QPainterPath path;
        double step = vertical ? h / n : w / n;

        if (vertical) {
            double startX = isLeft ? w : 0;
            path.moveTo(startX, 0);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = isLeft ? w - val * half * 0.9 : val * half * 0.9;
                double y = i * step + step / 2;
                path.lineTo(x, y);
            }
            path.lineTo(startX, h);
            path.closeSubpath();
        } else {
            double startY = isLeft ? h : 0;
            path.moveTo(0, startY);
            for (int i = 0; i < n; ++i) {
                int idx = (i * data.size()) / n;
                double val = std::clamp(data[idx], 0.0, 1.0);
                double x = i * step + step / 2;
                double y = isLeft ? h - val * half * 0.9 : val * half * 0.9;
                path.lineTo(x, y);
            }
            path.lineTo(w, startY);
            path.closeSubpath();
        }

        QColor g0 = lerpColor(0.0); g0.setAlpha(50);
        QColor g1 = lerpColor(0.5); g1.setAlpha(100);
        QColor g2 = lerpColor(1.0); g2.setAlpha(170);

        QLinearGradient grad;
        if (vertical)
            grad = QLinearGradient(isLeft ? w : 0, 0, isLeft ? w - half : half, 0);
        else
            grad = QLinearGradient(0, isLeft ? h : 0, 0, isLeft ? h - half : half);
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

    drawHill(left, true);
    drawHill(right, false);
}
