#ifndef VISUALEFFECT_H
#define VISUALEFFECT_H

#include <QVector>
#include <QColor>
#include <QPainter>
#include <QString>

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
    QColor colorFrom() const { return m_colorFrom; }
    QColor colorTo() const { return m_colorTo; }

protected:
    QColor lerpColor(double t) const
    {
        int r = m_colorFrom.red() + (m_colorTo.red() - m_colorFrom.red()) * t;
        int g = m_colorFrom.green() + (m_colorTo.green() - m_colorFrom.green()) * t;
        int b = m_colorFrom.blue() + (m_colorTo.blue() - m_colorFrom.blue()) * t;
        return QColor(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
    }

    QColor m_colorFrom{0, 180, 255};
    QColor m_colorTo{255, 0, 128};
};

#endif
