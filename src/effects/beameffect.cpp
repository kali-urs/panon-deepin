#include "beameffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BeamEffect::BeamEffect() = default;

void BeamEffect::render(QPainter &p, const QRectF &rect,
                          const QVector<double> &left,
                          const QVector<double> &,
                          const QVector<double> &,
                          bool vertical)
{
    if (left.isEmpty()) return;

    double w = rect.width();
    double h = rect.height();
    int n = std::min(128, static_cast<int>(left.size()));

    QPainterPath fillPath;
    QPainterPath linePath;

    if (vertical) {
        double step = h / n;
        fillPath.moveTo(w, 0);
        for (int i = 0; i < n; ++i) {
            int idx = (i * left.size()) / n;
            double val = std::clamp(left[idx], 0.0, 1.0);
            double x = w - val * val * w * 0.95;
            double y = i * step;
            if (i == 0) {
                fillPath.lineTo(x, y);
                linePath.moveTo(x, y);
            } else {
                fillPath.lineTo(x, y);
                linePath.lineTo(x, y);
            }
        }
        fillPath.lineTo(w, h);
        fillPath.closeSubpath();
    } else {
        double step = w / n;
        fillPath.moveTo(0, h);
        for (int i = 0; i < n; ++i) {
            int idx = (i * left.size()) / n;
            double val = std::clamp(left[idx], 0.0, 1.0);
            double x = i * step;
            double y = h - val * val * h * 0.95;
            if (i == 0) {
                fillPath.lineTo(x, y);
                linePath.moveTo(x, y);
            } else {
                fillPath.lineTo(x, y);
                linePath.lineTo(x, y);
            }
        }
        fillPath.lineTo(w, h);
        fillPath.closeSubpath();
    }

    QColor fillColor = lerpColor(0.4);
    fillColor.setAlpha(40);
    p.setBrush(fillColor);
    p.setPen(Qt::NoPen);
    p.drawPath(fillPath);

    QColor glowColor = lerpColor(0.6);
    glowColor.setAlpha(25);
    p.setPen(QPen(glowColor, 10));
    p.setBrush(Qt::NoBrush);
    p.drawPath(linePath);

    QColor lineColor = lerpColor(0.7);
    lineColor.setAlpha(180);
    p.setPen(QPen(lineColor, 2));
    p.drawPath(linePath);

    QColor brightLine = lerpColor(0.9);
    brightLine.setAlpha(100);
    p.setPen(QPen(brightLine, 1));
    p.drawPath(linePath);
}
