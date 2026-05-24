#ifndef BAREFFECT_H
#define BAREFFECT_H

#include "visualeffect.h"

class BarEffect : public VisualEffect
{
public:
    explicit BarEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &spectrum,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "Bars"; }
    void reset() override { m_peakHold.fill(0); }

    void setBarCount(int n) { m_barCount = std::clamp(n, 4, 128); }
    int barCount() const { return m_barCount; }

private:
    int m_barCount = 32;
    QVector<double> m_peakHold;
    double m_decayRate = 0.70;
};

#endif
