#include "panonplugin.h"
#include "spectrumwidget.h"
#include "audiosource.h"
#include "fftprocessor.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <algorithm>
#include <QLabel>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QTimer>


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
    return "Panon 音频可视化";
}

void PanonPlugin::setWidth(int w)
{
    m_width = std::clamp(w, 50, 600);
    m_widget->setFixedWidth(m_width);
    if (m_proxyInter)
        m_proxyInter->itemUpdate(this, pluginName());
}

void PanonPlugin::reapplyWidth()
{
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

    m_audioSource = new AudioSource(this);

    connect(m_audioSource, &AudioSource::samplesReady,
            this, &PanonPlugin::onSamplesReady);
    connect(m_audioSource, &AudioSource::stereoReady,
            this, &PanonPlugin::onStereoReady);
    connect(m_audioSource, &AudioSource::waveformReady,
            this, &PanonPlugin::onWaveformReady);

    if (m_audioSource->startCapture()) {
        qDebug() << "Panon: audio capture started, source:" << m_audioSource->currentSource();
    } else {
        qWarning() << "Panon: failed to start audio capture";
    }

    qDebug() << "Panon: available monitor sources:" << AudioSource::listMonitorSources();

    m_proxyInter->itemAdded(this, pluginName());

    QTimer::singleShot(200, this, &PanonPlugin::reapplyWidth);
}

QWidget *PanonPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_widget;
}

QWidget *PanonPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    auto *label = new QLabel("Panon 音频可视化");
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
    pauseItem["itemText"] = m_paused ? "继续" : "暂停";
    pauseItem["isActive"] = true;
    items.append(pauseItem);

    QJsonObject effectHeader;
    effectHeader["itemId"] = "effect_header";
    effectHeader["itemText"] = "[效果]";
    effectHeader["isActive"] = false;
    items.append(effectHeader);

    for (int i = 0; i < m_widget->effectCount(); ++i) {
        QJsonObject sub;
        sub["itemId"] = QString("effect_%1").arg(i);
        sub["itemText"] = (i == m_widget->effectIndex() ? "✓ " : "") + m_widget->effectName(i);
        sub["isActive"] = true;
        items.append(sub);
    }

    QJsonObject colorHeader;
    colorHeader["itemId"] = "color_header";
    colorHeader["itemText"] = "[颜色]";
    colorHeader["isActive"] = false;
    items.append(colorHeader);

    {
        QJsonObject sub;
        sub["itemId"] = "color_static";
        sub["itemText"] = QString(m_widget->colorMode() == SpectrumWidget::Static ? "✓ " : "") + "静态";
        sub["isActive"] = true;
        items.append(sub);
    }
    {
        QJsonObject sub;
        sub["itemId"] = "color_shift";
        sub["itemText"] = QString(m_widget->colorMode() == SpectrumWidget::Shift ? "✓ " : "") + "幻彩";
        sub["isActive"] = true;
        items.append(sub);
    }

    QJsonObject widthHeader;
    widthHeader["itemId"] = "width_header";
    widthHeader["itemText"] = "[宽度]";
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

    QJsonObject srcHeader;
    srcHeader["itemId"] = "src_header";
    srcHeader["itemText"] = "[音频源]";
    srcHeader["isActive"] = false;
    items.append(srcHeader);

    QString curSrc = m_audioSource->currentSource();
    QStringList srcWithState = AudioSource::listMonitorSourcesWithState();
    if (srcWithState.isEmpty()) {
        QJsonObject sub;
        sub["itemId"] = "no_sources";
        sub["itemText"] = "(无监控源)";
        sub["isActive"] = false;
        items.append(sub);
    } else {
        for (const QString &entry : srcWithState) {
            QStringList parts = entry.split('\t');
            if (parts.size() < 2) continue;
            QString name = parts[0];
            QString state = parts[1];
            bool isCurrent = (name == curSrc);
            QJsonObject sub;
            sub["itemId"] = "source_" + name;
            sub["itemText"] = (isCurrent ? "✓ " : "") + name + "  [" + state + "]";
            sub["isActive"] = true;
            items.append(sub);
        }
    }

    QJsonObject quitItem;
    quitItem["itemId"] = "quit";
    quitItem["itemText"] = "退出";
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
    } else if (menuId == "color_static") {
        m_widget->setColorMode(SpectrumWidget::Static);
    } else if (menuId == "color_shift") {
        m_widget->setColorMode(SpectrumWidget::Shift);
    } else if (menuId.startsWith("width_")) {
        bool ok = false;
        int w = QStringView(menuId).sliced(6).toInt(&ok);
        if (ok) setWidth(w);
    } else if (menuId.startsWith("source_")) {
        QString srcName = menuId.sliced(7);
        if (srcName != m_audioSource->currentSource()) {
            qDebug() << "Panon: switching audio source to:" << srcName;
            m_audioSource->switchSource(srcName);
        }
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

static void processChannel(const QVector<double> &input,
                           QVector<double> &barsOut,
                           FFTProcessor *fft)
{
    QVector<double> windowed = input;
    FFTProcessor::applyHanningWindow(windowed);

    QVector<double> rawMagnitudes;
    fft->process(windowed, rawMagnitudes);

    int magSize = rawMagnitudes.size();
    const int barCount = barsOut.size();
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
        val = std::sqrt(std::clamp(val * 200.0, 0.0, 1.0));
        if (val < 0.03) val = 0.03;
        barsOut[i] = val;
    }
}

void PanonPlugin::onSamplesReady(const QVector<double> &samples)
{
    static int frameCount = 0;
    ++frameCount;
    double maxSample = 0;
    for (double s : samples) maxSample = std::max(maxSample, std::abs(s));

    if (frameCount % 50 == 0) {
        qDebug() << "Panon: audio level =" << maxSample;
    }

    m_widget->updateWaveform(samples);
}

void PanonPlugin::onStereoReady(const QVector<double> &left, const QVector<double> &right)
{
    static int fftFrames = 0;
    ++fftFrames;
    const int barCount = 32;

    QVector<double> leftBars(barCount);
    QVector<double> rightBars(barCount);

    processChannel(left, leftBars, m_fft);
    processChannel(right, rightBars, m_fft);

    if (fftFrames <= 5) {
        qDebug() << "Panon: L bars:" << leftBars.mid(0, 4) << "R bars:" << rightBars.mid(0, 4);
    }

    m_widget->updateSpectrum(leftBars, rightBars);

    if (m_proxyInter) {
        m_proxyInter->itemUpdate(this, pluginName());
    }
}
