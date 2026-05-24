#include "panonplugin.h"
#include "spectrumwidget.h"
#include "audiosource.h"
#include "fftprocessor.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>


TrayIcon::TrayIcon(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(26, 26);
}

void TrayIcon::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor bg = palette().window().color();
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect(), 4, 4);

    double h = rect().height() * 0.7 * m_level;
    double w = rect().width() * 0.5;
    double x = (rect().width() - w) / 2;
    double y = rect().height() - h;

    QColor barColor(0, 180, 255, 200);
    p.setBrush(barColor);
    p.drawRoundedRect(QRectF(x, y, w, h), 2, 2);
}


PanonPlugin::PanonPlugin(QObject *parent)
    : QObject(parent)
{
}

PanonPlugin::~PanonPlugin()
{
    if (m_audioSource) {
        m_audioSource->stopCapture();
        m_audioSource->wait(2000);
    }
}

const QString PanonPlugin::pluginName() const
{
    return "panon";
}

const QString PanonPlugin::pluginDisplayName() const
{
    return "Panon Audio Visualizer";
}

void PanonPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    m_fft = new FFTProcessor(this);
    m_fft->setFFTSize(1024);

    m_widget = new SpectrumWidget;
    m_widget->setBarCount(32);
    m_widget->setFixedSize(300, 200);

    m_trayIcon = new TrayIcon;

    m_audioSource = new AudioSource(this);

    connect(m_audioSource, &AudioSource::samplesReady,
            this, &PanonPlugin::onSamplesReady);
    connect(m_audioSource, &AudioSource::waveformReady,
            this, &PanonPlugin::onWaveformReady);

    if (m_audioSource->startCapture()) {
        qDebug() << "Panon: audio capture started";
    } else {
        qWarning() << "Panon: failed to start audio capture";
    }

    m_proxyInter->itemAdded(this, pluginName());
}

QWidget *PanonPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_trayIcon;
}

QWidget *PanonPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    auto *label = new QLabel("Panon Audio Visualizer\nClick to open visualizer");
    label->setStyleSheet("padding: 8px;");
    return label;
}

QWidget *PanonPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_widget;
}

const QString PanonPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    QJsonObject menu;
    QJsonArray items;

    QJsonObject pauseItem;
    pauseItem["itemId"] = "pause";
    pauseItem["itemText"] = m_paused ? "Resume" : "Pause";
    pauseItem["isActive"] = true;
    items.append(pauseItem);

    QJsonObject effectHeader;
    effectHeader["itemId"] = "effect_header";
    effectHeader["itemText"] = "[Effects]";
    effectHeader["isActive"] = false;
    items.append(effectHeader);

    for (int i = 0; i < m_widget->effectCount(); ++i) {
        QJsonObject sub;
        sub["itemId"] = QString("effect_%1").arg(i);
        sub["itemText"] = (i == m_widget->effectIndex() ? "✓ " : "") + m_widget->effectName(i);
        sub["isActive"] = true;
        items.append(sub);
    }

    QJsonObject quitItem;
    quitItem["itemId"] = "quit";
    quitItem["itemText"] = "Quit";
    quitItem["isActive"] = true;
    items.append(quitItem);

    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;

    return QJsonDocument(menu).toJson(QJsonDocument::Compact);
}

void PanonPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey);
    Q_UNUSED(checked);

    if (menuId == "pause") {
        m_paused = !m_paused;
    } else if (menuId == "quit") {
        qApp->quit();
    } else if (menuId.startsWith("effect_")) {
        bool ok = false;
        int idx = QStringView(menuId).sliced(7).toInt(&ok);
        if (ok) m_widget->setEffect(idx);
    }
}

void PanonPlugin::positionChanged(const Dock::Position position)
{
    Q_UNUSED(position);
    updateOrientation();
}

void PanonPlugin::updateOrientation()
{
    bool vertical = (position() == Dock::Position::Left
                     || position() == Dock::Position::Right);
    m_widget->setOrientation(vertical);
}

void PanonPlugin::onWaveformReady(const QVector<double> &waveform)
{
    m_widget->updateWaveform(waveform);

    double level = 0;
    for (double s : waveform)
        level += std::abs(s);
    level = std::clamp(level / waveform.size() * 3.0, 0.0, 1.0);
    m_trayIcon->setLevel(level);
}

void PanonPlugin::onSamplesReady(const QVector<double> &samples)
{
    if (m_paused) return;

    QVector<double> windowed = samples;
    FFTProcessor::applyHanningWindow(windowed);

    QVector<double> rawMagnitudes;
    m_fft->process(windowed, rawMagnitudes);

    int magSize = rawMagnitudes.size();
    QVector<double> smoothed(magSize);
    const int barCount = 32;
    int binSize = qMax(1, magSize / barCount);

    for (int i = 0; i < barCount; ++i) {
        double sum = 0;
        int count = 0;
        int start = i * binSize;
        int end = qMin(start + binSize, magSize);
        for (int j = start; j < end; ++j) {
            sum += rawMagnitudes[j];
            count++;
        }
        double val = count > 0 ? sum / count : 0;
        val = std::log10(1.0 + val * 50) / 2.0;

        if (i < m_lastMagnitudes.size()) {
            val = val * 0.4 + m_lastMagnitudes[i] * 0.6;
        }

        smoothed[i] = val;
    }

    m_lastMagnitudes = smoothed;
    m_widget->updateSpectrum(smoothed);

    if (m_proxyInter) {
        m_proxyInter->itemUpdate(this, pluginName());
    }
}
