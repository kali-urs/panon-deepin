#ifndef WAVEEFFECT_H
#define WAVEEFFECT_H

#include "visualeffect.h"

class WaveEffect : public VisualEffect
{
public:
    explicit WaveEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &left,
                const QVector<double> &right,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "波形"; }
};

#endif
