#include "spectrumwidget.h"
#include "effects/bareffect.h"
#include "effects/waveeffect.h"
#include "effects/solideffect.h"
#include "effects/beameffect.h"
#include "effects/hilleffect.h"
#include "effects/spectrogrameffect.h"
#include <QPainter>
#include <algorithm>
#include <cmath>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(20, 20);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    initEffects();
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
}

void SpectrumWidget::deleteEffects()
{
    qDeleteAll(m_effects);
    m_effects.clear();
}

void SpectrumWidget::updateSpectrum(const QVector<double> &magnitudes)
{
    QMutexLocker lock(&m_mutex);
    m_spectrum = magnitudes;
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

void SpectrumWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QMutexLocker lock(&m_mutex);

    if (m_effects.isEmpty()) return;

    VisualEffect *effect = m_effects[m_effectIndex];
    effect->setColorFrom(m_colorFrom);
    effect->setColorTo(m_colorTo);

    effect->render(p, rect(), m_spectrum, m_waveform, m_vertical);
}
