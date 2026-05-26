#ifndef BALLEFFECT_H
#define BALLEFFECT_H

#include "visualeffect.h"

class BallEffect : public VisualEffect
{
public:
    explicit BallEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &left,
                const QVector<double> &right,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return QStringLiteral("弹球"); }
    void reset() override;

    void setBarCount(int n) { m_barCount = std::clamp(n, 4, 128); }

private:
    static QColor energyColor(double t);
    void updateBallPhysics(int idx, double barExtent, double h, double barCenter, double prevExtent);

    int m_barCount = 32;
    QVector<double> m_ballPos;
    QVector<double> m_ballVel;
    QVector<double> m_prevExtent;
    double m_gravity = 0.25;
    double m_launchRatio = 0.5;
};

#endif
