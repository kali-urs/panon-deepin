#ifndef HILLEFFECT_H
#define HILLEFFECT_H

#include "visualeffect.h"

class HillEffect : public VisualEffect
{
public:
    explicit HillEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &left,
                const QVector<double> &right,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "山丘"; }
};

#endif
