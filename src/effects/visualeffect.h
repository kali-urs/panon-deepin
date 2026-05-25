#ifndef VISUALEFFECT_H
#define VISUALEFFECT_H

#include <QVector>
#include <QColor>
#include <QPainter>
#include <QString>
#include <cmath>

class VisualEffect
{
public:
    VisualEffect() = default;
    virtual ~VisualEffect() = default;

    virtual void render(QPainter &p, const QRectF &rect,
                        const QVector<double> &left,
                        const QVector<double> &right,
                        const QVector<double> &waveform,
                        bool vertical) = 0;

    virtual QString name() const = 0;
    virtual void reset() {}

    void setColorFrom(const QColor &c) { m_colorFrom = c; }
    void setColorTo(const QColor &c) { m_colorTo = c; }
    void setHueShift(double deg) { m_hueShift = deg; }
    QColor colorFrom() const { return m_colorFrom; }
    QColor colorTo() const { return m_colorTo; }

protected:
    QColor lerpColor(double t) const
    {
        int r = m_colorFrom.red() + (m_colorTo.red() - m_colorFrom.red()) * t;
        int g = m_colorFrom.green() + (m_colorTo.green() - m_colorFrom.green()) * t;
        int b = m_colorFrom.blue() + (m_colorTo.blue() - m_colorFrom.blue()) * t;
        QColor c(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
        if (m_hueShift != 0.0) {
            int h = c.hslHue();
            if (h < 0) h = 0;
            c.setHsl((h + static_cast<int>(m_hueShift)) % 360, c.hslSaturation(), c.lightness());
        }
        return c;
    }

    QColor m_colorFrom{0, 180, 255};
    QColor m_colorTo{255, 0, 128};
    double m_hueShift = 0.0;
};

#endif
