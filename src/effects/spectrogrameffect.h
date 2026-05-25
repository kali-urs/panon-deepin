#ifndef SPECTROGRAMEFFECT_H
#define SPECTROGRAMEFFECT_H

#include "visualeffect.h"
#include <QImage>
#include <QVector>
#include <QMutex>

class SpectrogramEffect : public VisualEffect
{
public:
    explicit SpectrogramEffect();

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &left,
                const QVector<double> &right,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "频谱图"; }
    void reset() override;

private:
    void scrollImage();
    void addColumn(const QVector<double> &data, int xOffset);

    QImage m_image;
    QMutex m_mutex;
    int m_colCount = 0;
};

#endif
