#ifndef WAVEEFFECT_H
#define WAVEEFFECT_H

#include "visualeffect.h"

class WaveEffect : public VisualEffect
{
    Q_OBJECT
public:
    explicit WaveEffect(QObject *parent = nullptr);

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &spectrum,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "Wave"; }
};

#endif
