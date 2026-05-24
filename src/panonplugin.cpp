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

void PanonPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    m_fft = new FFTProcessor(this);
    m_fft->setFFTSize(1024);

    m_widget = new SpectrumWidget;
    m_widget->setBarCount(32);

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
    auto *label = new QLabel("Panon Audio Visualizer\nClick to pause/resume");
    label->setStyleSheet("padding: 8px;");
    return label;
}

QWidget *PanonPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    auto *label = new QLabel(m_paused ? "Paused" : "Listening...");
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("padding: 20px; font-size: 14px;");
    return label;
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
        m_widget->setEffect(idx);
    }
}

PluginFlags PanonPlugin::flags() const
{
    return PluginFlag::Type_Common
         | PluginFlag::Quick_Single
         | PluginFlag::Attribute_CanDrag
         | PluginFlag::Attribute_CanInsert
         | PluginFlag::Attribute_CanSetting;
}

void PanonPlugin::positionChanged(const Dock::Position position)
{
    Q_UNUSED(position);
    updateOrientation();
}

QIcon PanonPlugin::icon(const DockPart &part, DGuiApplicationHelper::ColorType themeType)
{
    Q_UNUSED(part);
    Q_UNUSED(themeType);
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor colors[] = {{0, 180, 255}, {0, 229, 255}, {255, 0, 128}, {255, 102, 0}};
    for (int i = 0; i < 4; ++i) {
        painter.fillRect(QRectF(2 + i * 6, 20 - (i + 1) * 4, 4, (i + 1) * 4), colors[i]);
    }
    painter.end();
    return QIcon(pix);
}

bool PanonPlugin::eventHandler(QEvent *event)
{
    Q_UNUSED(event);
    return false;
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
    if (m_paused) return;

    QVector<double> windowed = samples;
    FFTProcessor::applyHanningWindow(windowed);

    QVector<double> rawMagnitudes;
    m_fft->process(windowed, rawMagnitudes);

    QVector<double> smoothed(rawMagnitudes.size());
    const int barCount = 32;
    int binSize = std::max(1, static_cast<int>(rawMagnitudes.size()) / barCount);

    for (int i = 0; i < barCount; ++i) {
        double sum = 0;
        int count = 0;
        int start = i * binSize;
        int end = std::min(start + binSize, static_cast<int>(rawMagnitudes.size()));
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
