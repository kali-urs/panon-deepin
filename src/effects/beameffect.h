#ifndef BEAMEFFECT_H
#define BEAMEFFECT_H

#include "visualeffect.h"

class BeamEffect : public VisualEffect
{
public:
    explicit BeamEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &left,
                const QVector<double> &right,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "Beam"; }
};

#endif
