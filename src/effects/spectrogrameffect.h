#ifndef SPECTROGRAMEFFECT_H
#define SPECTROGRAMEFFECT_H

#include "visualeffect.h"
#include <QImage>
#include <QVector>
#include <QMutex>

class SpectrogramEffect : public VisualEffect
{
public:
    explicit SpectrogramEffect(QObject *parent = nullptr);

    void render(QPainter &p, const QRectF &rect,
                const QVector<double> &spectrum,
                const QVector<double> &waveform,
                bool vertical) override;
    QString name() const override { return "Spectrogram"; }
    void reset() override;

private:
    void scrollImage();
    void addColumn(const QVector<double> &spectrum);

    QImage m_image;
    QMutex m_mutex;
    int m_colCount = 0;
};

#endif
