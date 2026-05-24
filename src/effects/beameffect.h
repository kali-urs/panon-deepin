#ifndef BEAMEFFECT_H
#define BEAMEFFECT_H

#include "visualeffect.h"

class BeamEffect : public VisualEffect
{
    Q_OBJECT
public:
    explicit BeamEffect(QObject *parent = nullptr);

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &spectrum,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "Beam"; }
};

#endif
