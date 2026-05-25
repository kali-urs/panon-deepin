#include "beameffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BeamEffect::BeamEffect() = default;

void BeamEffect::render(QPainter &p, const QRectF &rect,
                          const QVector<double> &left,
                          const QVector<double> &right,
                          const QVector<double> &,
                          bool vertical)
{
    if (left.isEmpty() || right.isEmpty()) return;

    double w = rect.width();
    double h = rect.height();
    double half = vertical ? w * 0.5 : h * 0.5;
    int n = std::min(32, static_cast<int>(left.size()));

    auto drawBeams = [&](const QVector<double> &data, bool isLeft) {
        double step = vertical ? h / n : w / n;
        double gap = step * 0.2;

        for (int i = 0; i < n; ++i) {
            int idx = (i * data.size()) / n;
            double val = std::clamp(data[idx], 0.0, 1.0);
            if (val < 0.01) continue;

            double t = (double)i / std::max(1, n - 1);
            QColor color = lerpColor(t);
            color.setAlpha(200);

            QPainterPath beam;
            if (vertical) {
                double barX, barY = i * step + gap / 2;
                double barH = step - gap;
                if (isLeft) {
                    barX = w - val * half * 0.9;
                    beam.addRoundedRect(QRectF(barX, barY, w - barX, barH), 0, 2);
                } else {
                    barX = 0;
                    beam.addRoundedRect(QRectF(0, barY, val * half * 0.9, barH), 0, 2);
                }
            } else {
                double barX = i * step + gap / 2;
                double barW = std::max(1.0, step - gap);
                if (isLeft) {
                    double barH = val * half * 0.9;
                    beam.addRoundedRect(QRectF(barX, h - barH, barW, barH), 2, 0);
                } else {
                    double barH = val * half * 0.9;
                    beam.addRoundedRect(QRectF(barX, 0, barW, barH), 2, 0);
                }
            }

            QColor glow = color;
            glow.setAlpha(40);
            p.setPen(QPen(glow, 6));
            p.setBrush(Qt::NoBrush);
            p.drawPath(beam);
            p.fillPath(beam, color);
        }
    };

    drawBeams(left, true);
    drawBeams(right, false);
}
