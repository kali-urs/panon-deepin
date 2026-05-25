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
    int n = std::min(32, static_cast<int>(left.size()));

    auto drawBeams = [&](const QVector<double> &data, bool isLeft, bool vertical) {
        double half = vertical ? h * 0.5 : w * 0.5;
        double step = vertical ? half / n : half / n;
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
                double barH = std::max(1.0, step - gap);
                double barY = isLeft ? i * step + gap / 2
                                     : h - (i * step + gap / 2) - barH;
                double barW = val * w * 0.9;
                beam.addRoundedRect(QRectF(0, barY, barW, barH), 0, 2);
            } else {
                double barW = std::max(1.0, step - gap);
                double barX = isLeft ? i * step + gap / 2
                                     : w - (i * step + gap / 2) - barW;
                double barH = val * h * 0.9;
                beam.addRoundedRect(QRectF(barX, h - barH, barW, barH), 2, 0);
            }

            QColor glow = color;
            glow.setAlpha(40);
            p.setPen(QPen(glow, 6));
            p.setBrush(Qt::NoBrush);
            p.drawPath(beam);
            p.fillPath(beam, color);
        }
    };

    drawBeams(left, true, vertical);
    drawBeams(right, false, vertical);
}
