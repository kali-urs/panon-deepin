#include "beameffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BeamEffect::BeamEffect(QObject *parent)
    : VisualEffect(parent)
{
}

void BeamEffect::render(QPainter &p, const QRectF &rect,
                         const QVector<double> &spectrum,
                         const QVector<double> &,
                         bool vertical)
{
    p.fillRect(rect, QColor(0, 0, 0, 40));

    if (spectrum.isEmpty()) {
        p.setPen(QColor(100, 100, 100));
        p.drawText(rect, Qt::AlignCenter, "No audio");
        return;
    }

    double w = rect.width();
    double h = rect.height();
    int n = spectrum.size();

    QPainterPath fillPath;
    QPainterPath linePath;

    if (vertical) {
        double step = h / n;
        double baseX = w;
        fillPath.moveTo(baseX, 0);
        for (int i = 0; i < n; ++i) {
            double val = std::clamp(spectrum[i], 0.0, 1.0);
            double x = w - val * w * 0.9;
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
        double baseY = h;
        fillPath.moveTo(0, baseY);
        for (int i = 0; i < n; ++i) {
            double val = std::clamp(spectrum[i], 0.0, 1.0);
            double x = i * step;
            double y = h - val * h * 0.9;
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

    QColor fill = lerpColor(0.5);
    fill.setAlpha(60);
    p.setBrush(fill);
    p.setPen(Qt::NoPen);
    p.drawPath(fillPath);

    QColor line = lerpColor(0.7);
    line.setAlpha(200);
    p.setPen(QPen(line, 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(linePath);
}
