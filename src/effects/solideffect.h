#ifndef SOLIDEFFECT_H
#define SOLIDEFFECT_H

#include "visualeffect.h"

class SolidEffect : public VisualEffect
{
public:
    explicit SolidEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &left,
                const QVector<double> &right,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "实心"; }
};

#endif
