#include "balleffect.h"
#include <QPainterPath>
#include <algorithm>
#include <cmath>

BallEffect::BallEffect() = default;

QColor BallEffect::energyColor(double t)
{
    t = std::clamp(t, 0.0, 1.0);
    if (t < 0.15) {
        double u = t / 0.15;
        return QColor(0, static_cast<int>(60 + u * 160), static_cast<int>(80 + u * 175));
    }
    if (t < 0.35) {
        double u = (t - 0.15) / 0.2;
        return QColor(static_cast<int>(u * 100), 220, 255);
    }
    if (t < 0.55) {
        double u = (t - 0.35) / 0.2;
        return QColor(static_cast<int>(100 + u * 155), static_cast<int>(220 - u * 50), static_cast<int>(255 - u * 180));
    }
    if (t < 0.75) {
        double u = (t - 0.55) / 0.2;
        return QColor(255, static_cast<int>(170 - u * 100), static_cast<int>(75 - u * 75));
    }
    double u = (t - 0.75) / 0.25;
    return QColor(255, static_cast<int>(70 + u * 185), static_cast<int>(0 + u * 200));
}

void BallEffect::reset()
{
    m_ballPos.clear();
    m_ballVel.clear();
    m_prevExtent.clear();
}

void BallEffect::updateBallPhysics(int idx, double barExtent, double h, double barCenter, double prevExtent)
{
    double ballSize = 4.0;
    if (idx >= m_ballPos.size()) return;

    if (barExtent > m_ballPos[idx]) {
        double vel = (barExtent - prevExtent) * m_launchRatio;
        m_ballPos[idx] = barExtent;
        m_ballVel[idx] = std::max(-10.0, -vel);
    } else {
        m_ballVel[idx] += m_gravity;
        m_ballPos[idx] += m_ballVel[idx];

        if (m_ballPos[idx] <= barExtent) {
            m_ballPos[idx] = barExtent;
            m_ballVel[idx] = 0;
        }
    }
    if (m_ballPos[idx] < 0) m_ballPos[idx] = 0;
}

void BallEffect::render(QPainter &p, const QRectF &rect,
                         const QVector<double> &left,
                         const QVector<double> &right,
                         const QVector<double> &,
                         bool vertical)
{
    int n = std::min<int>(m_barCount, std::min<int>(left.size(), right.size()));
    int total = n * 2;
    if (m_ballPos.size() != total) {
        m_ballPos.resize(total, 0);
        m_ballVel.resize(total, 0);
        m_prevExtent.resize(total, 0);
    }

    double w = rect.width();
    double h = rect.height();
    double gap = 0.15;
    double radius = 2.0;
    double ballSize = 4.0;

    auto drawBar = [&](int idx, double val, double barX, double barY, double barW, double barH) {
        if (val < 0.01) val = 0;
        if (barW > 0.5 && barH > 0.5) {
            QColor c = energyColor(val);
            c.setAlpha(220);
            QRectF r(barX, barY, barW, barH);
            QPainterPath path;
            path.addRoundedRect(r, radius, radius);
            p.fillPath(path, c);
        }

        double extent = vertical ? barX + barW : h - barY;
        double barCenter = vertical ? barY + barH / 2 : barX + barW / 2;
        updateBallPhysics(idx, extent, h, barCenter, m_prevExtent[idx]);
        m_prevExtent[idx] = extent;

        double ballX, ballY;
        if (vertical) {
            ballX = m_ballPos[idx];
            ballY = barCenter;
        } else {
            ballX = barCenter;
            ballY = m_ballPos[idx];
        }
        if (ballY > 0 && ballY < h && ballX > 0 && ballX < w) {
            p.setBrush(Qt::white);
            p.setPen(QPen(QColor(255, 255, 255, 60), 4));
            p.drawEllipse(QPointF(ballX, ballY), ballSize, ballSize);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(ballX, ballY), ballSize * 0.6, ballSize * 0.6);
        }
    };

    if (vertical) {
        double half = h * 0.5;
        double unitH = half / n;
        for (int i = 0; i < n; ++i) {
            int idx = (i * left.size()) / n;
            double lVal = std::clamp(left[idx], 0.0, 1.0);
            double rVal = std::clamp(right[idx], 0.0, 1.0);
            if (lVal < 0.01) lVal = 0;
            if (rVal < 0.01) rVal = 0;

            double barH = std::max(1.0, unitH - unitH * gap);
            double barY = i * unitH + gap / 2;

            double lBarW = lVal * w * 0.92;
            drawBar(i, lVal, 0, barY, lBarW, barH);

            double rBarW = rVal * w * 0.92;
            double rBarY = h - (i * unitH + gap / 2) - barH;
            drawBar(n + i, rVal, 0, rBarY, rBarW, barH);
        }
        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 25), 1));
            p.drawLine(QPointF(0, half), QPointF(w, half));
        }
    } else {
        double half = w * 0.5;
        double unitW = half / n;
        for (int i = 0; i < n; ++i) {
            int idx = (i * left.size()) / n;
            double lVal = std::clamp(left[idx], 0.0, 1.0);
            double rVal = std::clamp(right[idx], 0.0, 1.0);
            if (lVal < 0.01) lVal = 0;
            if (rVal < 0.01) rVal = 0;

            double barW = std::max(1.0, unitW - unitW * gap);
            double barX = i * unitW + gap / 2;

            double lBarH = lVal * h * 0.92;
            drawBar(i, lVal, barX, h - lBarH, barW, lBarH);

            double rBarX = w - (i * unitW + gap / 2) - barW;
            double rBarH = rVal * h * 0.92;
            drawBar(n + i, rVal, rBarX, h - rBarH, barW, rBarH);
        }
        if (half > 2) {
            p.setPen(QPen(QColor(255, 255, 255, 25), 1));
            p.drawLine(QPointF(half, 0), QPointF(half, h));
        }
    }
}
