#include "spectrumwidget.h"
#include "effects/bareffect.h"
#include "effects/waveeffect.h"
#include "effects/solideffect.h"
#include "effects/beameffect.h"
#include "effects/hilleffect.h"
#include "effects/spectrogrameffect.h"
#include "effects/balleffect.h"
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <QRandomGenerator>
#include <QFontMetrics>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(80, 24);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    initEffects();

    m_colorTimer = new QTimer(this);
    connect(m_colorTimer, &QTimer::timeout, this, &SpectrumWidget::advanceColorShift);
    m_colorTimer->start(80);
}

SpectrumWidget::~SpectrumWidget()
{
    deleteEffects();
}

void SpectrumWidget::initEffects()
{
    m_effects.append(new BarEffect());
    m_effects.append(new WaveEffect());
    m_effects.append(new SolidEffect());
    m_effects.append(new BeamEffect());
    m_effects.append(new HillEffect());
    m_effects.append(new SpectrogramEffect());
    m_effects.append(new BallEffect());
}

void SpectrumWidget::deleteEffects()
{
    qDeleteAll(m_effects);
    m_effects.clear();
}

void SpectrumWidget::updateSpectrum(const QVector<double> &left, const QVector<double> &right)
{
    QMutexLocker lock(&m_mutex);
    m_spectrumLeft = left;
    m_spectrumRight = right;
    update();
}

void SpectrumWidget::updateWaveform(const QVector<double> &waveform)
{
    QMutexLocker lock(&m_mutex);
    m_waveform = waveform;
}

void SpectrumWidget::setBarCount(int count)
{
    m_barCount = std::max(4, count);
    update();
}

void SpectrumWidget::setColors(const QColor &from, const QColor &to)
{
    m_colorFrom = from;
    m_colorTo = to;
    update();
}

void SpectrumWidget::setOrientation(bool vertical)
{
    m_vertical = vertical;
    updateGeometry();
}

void SpectrumWidget::setEffect(int index)
{
    if (index >= 0 && index < m_effects.size()) {
        if (m_effectIndex < m_effects.size())
            m_effects[m_effectIndex]->reset();
        m_effectIndex = index;
        m_effects[m_effectIndex]->reset();
        update();
    }
}

int SpectrumWidget::effectCount() const
{
    return m_effects.size();
}

QString SpectrumWidget::effectName(int index) const
{
    if (index >= 0 && index < m_effects.size())
        return m_effects[index]->name();
    return {};
}

void SpectrumWidget::advanceColorShift()
{
    if (m_colorMode != Shift) return;
    m_hueShift = std::fmod(m_hueShift + QRandomGenerator::global()->bounded(12) - 6, 360.0);
    update();
}

void SpectrumWidget::setDownloadProgress(int percent, bool active)
{
    m_downloadPercent = std::clamp(percent, 0, 100);
    m_downloadActive = active;
    update();
}

void SpectrumWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QMutexLocker lock(&m_mutex);

    if (m_effects.isEmpty()) return;

    VisualEffect *effect = m_effects[m_effectIndex];
    effect->setColorFrom(m_colorFrom);
    effect->setColorTo(m_colorTo);
    effect->setHueShift(m_hueShift);

    effect->render(p, rect(), m_spectrumLeft, m_spectrumRight, m_waveform, m_vertical);

    if (m_downloadActive) {
        int barH = 4;
        double w = rect().width();
        double h = rect().height();
        double fw = w * m_downloadPercent / 100.0;

        QRectF bar(0, h - barH, fw, barH);
        p.fillRect(bar, QColor(0, 180, 255, 200));

        if (w > 100) {
            p.setPen(QColor(255, 255, 255, 180));
            QFont f = p.font();
            f.setPixelSize(10);
            p.setFont(f);
            QRectF textRect(4, h - barH - 12, w - 8, barH + 12);
            p.drawText(textRect, Qt::AlignLeft | Qt::AlignBottom,
                       QString::number(m_downloadPercent) + "%");
        }
    }
}
