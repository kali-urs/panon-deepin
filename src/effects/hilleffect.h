#ifndef HILLEFFECT_H
#define HILLEFFECT_H

#include "visualeffect.h"

class HillEffect : public VisualEffect
{
public:
    explicit HillEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &spectrum,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "Hill"; }
};

#endif
