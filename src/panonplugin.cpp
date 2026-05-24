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

void PanonPlugin::setWidth(int w)
{
    m_width = std::clamp(w, 50, 600);
    m_widget->setFixedWidth(m_width);
    if (m_proxyInter)
        m_proxyInter->itemUpdate(this, pluginName());
}

void PanonPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    m_fft = new FFTProcessor(this);
    m_fft->setFFTSize(1024);

    m_widget = new SpectrumWidget;
    m_widget->setBarCount(32);
    m_widget->setFixedWidth(m_width);

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
    return m_widget;
}

QWidget *PanonPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    auto *label = new QLabel("Panon Audio Visualizer");
    label->setStyleSheet("padding: 8px;");
    return label;
}

QWidget *PanonPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return nullptr;
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

    QJsonObject widthHeader;
    widthHeader["itemId"] = "width_header";
    widthHeader["itemText"] = "[Width]";
    widthHeader["isActive"] = false;
    items.append(widthHeader);

    QList<int> widths = {100, 150, 200, 250, 300, 400};
    for (int w : widths) {
        QJsonObject sub;
        sub["itemId"] = QString("width_%1").arg(w);
        sub["itemText"] = (w == m_width ? "✓ " : "") + QString("%1px").arg(w);
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
    } else if (menuId.startsWith("width_")) {
        bool ok = false;
        int w = QStringView(menuId).sliced(6).toInt(&ok);
        if (ok) setWidth(w);
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
}

void PanonPlugin::onSamplesReady(const QVector<double> &samples)
{
    static int frameCount = 0;
    if (++frameCount % 50 == 0) {
        double maxVal = 0;
        for (double s : samples) maxVal = std::max(maxVal, std::abs(s));
        qDebug() << "Panon: audio level =" << maxVal;
    }

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
        val = val * 20.0;
        val = std::sqrt(std::clamp(val, 0.0, 1.0));

        if (i < m_lastMagnitudes.size()) {
            val = val * 0.85 + m_lastMagnitudes[i] * 0.15;
        }

        smoothed[i] = val;
    }

    m_lastMagnitudes = smoothed;
    m_widget->updateSpectrum(smoothed);

    if (m_proxyInter) {
        m_proxyInter->itemUpdate(this, pluginName());
    }
}
