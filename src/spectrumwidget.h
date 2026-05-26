#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QMutex>
#include <QString>
#include <QTimer>

class VisualEffect;

class SpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    enum ColorMode { Static, Shift };

    explicit SpectrumWidget(QWidget *parent = nullptr);
    ~SpectrumWidget() override;

    void updateSpectrum(const QVector<double> &left, const QVector<double> &right);
    void updateWaveform(const QVector<double> &waveform);
    void setBarCount(int count);
    void setColors(const QColor &from, const QColor &to);
    void setOrientation(bool vertical);

    void setEffect(int index);
    int effectIndex() const { return m_effectIndex; }
    int effectCount() const;
    QString effectName(int index) const;

    void setColorMode(ColorMode mode) { m_colorMode = mode; }
    ColorMode colorMode() const { return m_colorMode; }

    void setDownloadProgress(int percent, bool active);
    bool isDownloading() const { return m_downloadActive; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return m_vertical ? QSize(36, 150) : QSize(150, 36); }
    QSize minimumSizeHint() const override { return m_vertical ? QSize(20, 20) : QSize(20, 20); }

private:
    void initEffects();
    void deleteEffects();
    void advanceColorShift();

    QVector<double> m_spectrumLeft;
    QVector<double> m_spectrumRight;
    QVector<double> m_waveform;
    QMutex m_mutex;
    int m_barCount = 32;
    bool m_vertical = false;
    QColor m_colorFrom{0, 180, 255};
    QColor m_colorTo{255, 0, 128};

    QVector<VisualEffect *> m_effects;
    int m_effectIndex = 0;
    ColorMode m_colorMode = Static;
    double m_hueShift = 0.0;
    QTimer *m_colorTimer = nullptr;

    bool m_downloadActive = false;
    int m_downloadPercent = 0;
};

#endif
