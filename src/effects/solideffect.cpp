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
    p.fillRect(rect, QColor(13, 13, 26, 200));

    if (spectrum.isEmpty()) {
        p.setPen(QColor(100, 100, 120));
        p.drawText(rect, Qt::AlignCenter, "No audio");
        return;
    }

    double w = rect.width();
    double h = rect.height();
    int n = std::min(128, static_cast<int>(spectrum.size()));

    QPainterPath fillPath;

    if (vertical) {
        double step = h / n;
        fillPath.moveTo(w, 0);
        for (int i = 0; i < n; ++i) {
            int idx = (i * spectrum.size()) / n;
            double val = std::clamp(spectrum[idx], 0.0, 1.0);
            double x = w - val * w * 0.92;
            double y = i * step + step / 2;
            fillPath.lineTo(x, y);
        }
        fillPath.lineTo(w, h);
        fillPath.closeSubpath();
    } else {
        double step = w / n;
        fillPath.moveTo(0, h);
        for (int i = 0; i < n; ++i) {
            int idx = (i * spectrum.size()) / n;
            double val = std::clamp(spectrum[idx], 0.0, 1.0);
            double x = i * step + step / 2;
            double y = h - val * h * 0.92;
            fillPath.lineTo(x, y);
        }
        fillPath.lineTo(w, h);
        fillPath.closeSubpath();
    }

    QLinearGradient grad;
    if (vertical) {
        grad = QLinearGradient(0, 0, w, 0);
    } else {
        grad = QLinearGradient(0, 0, 0, h);
    }
    QColor g0 = lerpColor(0.0); g0.setAlpha(80);
    QColor g1 = lerpColor(0.5); g1.setAlpha(120);
    QColor g2 = lerpColor(1.0); g2.setAlpha(180);
    grad.setColorAt(0.0, g0);
    grad.setColorAt(0.5, g1);
    grad.setColorAt(1.0, g2);

    QColor fillGlow = lerpColor(0.5);
    fillGlow.setAlpha(25);
    p.setPen(QPen(fillGlow, 10));
    p.setBrush(Qt::NoBrush);
    p.drawPath(fillPath);

    p.setBrush(QBrush(grad));
    p.setPen(Qt::NoPen);
    p.drawPath(fillPath);

    QPainterPath linePath = fillPath;
    QColor lineColor = lerpColor(0.7);
    lineColor.setAlpha(200);
    p.setPen(QPen(lineColor, 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(linePath);
}
