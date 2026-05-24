#ifndef HILLEFFECT_H
#define HILLEFFECT_H

#include "visualeffect.h"

class HillEffect : public VisualEffect
{
    Q_OBJECT
public:
    explicit HillEffect(QObject *parent = nullptr);

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &spectrum,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "Hill"; }
};

#endif
