#include "panonplugin.h"
#include "spectrumwidget.h"
#include "audiosource.h"
#include "fftprocessor.h"
#include "updatechecker.h"

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
#include <QMessageBox>
#include <QProgressDialog>
#include <QDir>
#include <QProcess>
#include <QFile>


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
    saveSettings();
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
    m_widget->setBarCount(27);

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

    loadSettings();

    m_updateChecker = new UpdateChecker(PANON_VERSION, this);
    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &PanonPlugin::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::upToDate,
            this, &PanonPlugin::setUpToDate);
    connect(m_updateChecker, &UpdateChecker::networkError,
            this, &PanonPlugin::onUpdateError);

    m_periodicCheckTimer = new QTimer(this);
    connect(m_periodicCheckTimer, &QTimer::timeout, m_updateChecker, &UpdateChecker::check);
    m_periodicCheckTimer->start(3600000);

    QTimer::singleShot(5000, m_updateChecker, &UpdateChecker::check);

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

    QJsonObject updateHeader;
    updateHeader["itemId"] = "update_header";
    updateHeader["itemText"] = "[更新]";
    updateHeader["isActive"] = false;
    items.append(updateHeader);

    if (m_updateChecker && m_updateChecker->hasUpdate()) {
        QJsonObject updateItem;
        updateItem["itemId"] = "open_release";
        updateItem["itemText"] = QString("⬇ 更新到 v%1").arg(m_updateVersion);
        updateItem["isActive"] = true;
        items.append(updateItem);
    }

    {
        QJsonObject sub;
        sub["itemId"] = "check_update";
        sub["itemText"] = m_updateChecker && m_updateChecker->isChecking()
            ? "检查中…" : "检查更新";
        sub["isActive"] = !(m_updateChecker && m_updateChecker->isChecking());
        items.append(sub);
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
        if (ok) { m_widget->setEffect(idx); saveSettings(); }
    } else if (menuId == "color_static") {
        m_widget->setColorMode(SpectrumWidget::Static);
        saveSettings();
    } else if (menuId == "color_shift") {
        m_widget->setColorMode(SpectrumWidget::Shift);
        saveSettings();
    } else if (menuId.startsWith("width_")) {
        bool ok = false;
        int w = QStringView(menuId).sliced(6).toInt(&ok);
        if (ok) setWidth(w);
    } else if (menuId.startsWith("source_")) {
        QString srcName = menuId.sliced(7);
        if (srcName != m_audioSource->currentSource()) {
            qDebug() << "Panon: switching audio source to:" << srcName;
            m_audioSource->switchSource(srcName);
            saveSettings();
        }
    } else if (menuId == "check_update") {
        if (m_updateChecker)
            m_updateChecker->check();
    } else if (menuId == "open_release") {
        startUpdateDownload();
    }
}

void PanonPlugin::positionChanged(const Dock::Position position)
{
    Q_UNUSED(position);
    updateOrientation();
}

void PanonPlugin::loadSettings()
{
    QSettings s("kali-urs", "panon-deepin");
    m_width = s.value("width", 150).toInt();
    int idx = s.value("effect", 0).toInt();
    if (idx >= 0 && idx < m_widget->effectCount())
        m_widget->setEffect(idx);
    m_widget->setColorMode(static_cast<SpectrumWidget::ColorMode>(
        s.value("colorMode", 0).toInt()));
    QString src = s.value("source").toString();
    if (!src.isEmpty() && src != m_audioSource->currentSource())
        m_audioSource->switchSource(src);
}

void PanonPlugin::saveSettings()
{
    QSettings s("kali-urs", "panon-deepin");
    s.setValue("width", m_width);
    s.setValue("effect", m_widget->effectIndex());
    s.setValue("colorMode", static_cast<int>(m_widget->colorMode()));
    s.setValue("source", m_audioSource->currentSource());
}

void PanonPlugin::onUpdateAvailable(const QString &latest)
{
    m_updateVersion = latest;
    if (m_updateDialogShown) return;
    m_updateDialogShown = true;

    QMessageBox msgBox(m_widget);
    msgBox.setWindowTitle("发现新版本");
    msgBox.setText(QString("Panon v%1 已发布！\n是否下载并自动安装？").arg(latest));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.addButton("下载并安装", QMessageBox::AcceptRole);
    msgBox.addButton("稍后提醒", QMessageBox::RejectRole);
    msgBox.exec();

    if (msgBox.buttonRole(msgBox.clickedButton()) == QMessageBox::AcceptRole) {
        startUpdateDownload();
    }
}

void PanonPlugin::setUpToDate()
{
    if (m_proxyInter)
        m_proxyInter->itemUpdate(this, pluginName());
}

void PanonPlugin::onUpdateError(const QString &msg)
{
    Q_UNUSED(msg);
}

void PanonPlugin::startUpdateDownload()
{
    if (m_downloadProc && m_downloadProc->state() != QProcess::NotRunning) {
        QMessageBox::information(m_widget, "下载中", "更新已在下载中，请稍候。");
        return;
    }

    m_downloadPath = QDir::tempPath() + "/dde-dock-panon_update.deb";
    QString url = m_updateChecker->downloadUrl();

    // Get content-length
    m_downloadTotal = 0;
    QProcess headProc;
    headProc.start("curl", {"-sI", url});
    if (headProc.waitForFinished(5000) && headProc.exitCode() == 0) {
        for (const QString &line : QString(headProc.readAllStandardOutput()).split('\n')) {
            if (line.startsWith("content-length:", Qt::CaseInsensitive)) {
                m_downloadTotal = line.mid(15).trimmed().toLongLong();
                break;
            }
        }
    }

    m_progressDlg = new QProgressDialog(
        "正在下载 Panon v" + m_updateVersion + " ...", "取消", 0, 0, m_widget);
    m_progressDlg->setWindowTitle("下载更新");
    m_progressDlg->setAutoClose(true);
    m_progressDlg->setAutoReset(false);
    m_progressDlg->setMinimumDuration(0);
    m_progressDlg->show();

    m_downloadProc = new QProcess(this);
    connect(m_downloadProc, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus) {
        onDownloadFinished(exitCode);
    });
    connect(m_progressDlg, &QProgressDialog::canceled, this, [this]() {
        if (m_downloadProc) m_downloadProc->kill();
    });

    m_downloadProc->start("curl", QStringList{"-L", "-o", m_downloadPath, url});

    if (m_downloadProgressTimer)
        m_downloadProgressTimer->stop();
    m_downloadProgressTimer = new QTimer(this);
    connect(m_downloadProgressTimer, &QTimer::timeout, this, [this]() {
        qint64 bytes = QFile(m_downloadPath).size();
        int pct = m_downloadTotal > 0
            ? static_cast<int>(bytes * 100 / m_downloadTotal)
            : 0;
        m_widget->setDownloadProgress(std::clamp(pct, 0, 100), true);
    });
    m_downloadProgressTimer->start(500);
}

void PanonPlugin::onDownloadFinished(int exitCode)
{
    if (m_progressDlg) {
        m_progressDlg->close();
        m_progressDlg->deleteLater();
        m_progressDlg = nullptr;
    }

    if (m_downloadProgressTimer) {
        m_downloadProgressTimer->stop();
        m_downloadProgressTimer->deleteLater();
        m_downloadProgressTimer = nullptr;
    }

    auto *proc = qobject_cast<QProcess *>(sender());
    if (proc != m_downloadProc) return;

    m_downloadProc = nullptr;

    if (exitCode != 0) {
        m_widget->setDownloadProgress(0, false);
        QMessageBox::warning(m_widget, "下载失败",
            "下载更新失败，请检查网络后重试。\n"
            "也可手动下载:\n" + m_updateChecker->downloadUrl());
        return;
    }

    m_widget->setDownloadProgress(100, false);

    QTimer::singleShot(200, this, [this]() {
        QMessageBox::StandardButton btn = QMessageBox::question(m_widget, "安装更新",
            "Panon v" + m_updateVersion + " 已下载完成，是否立即安装？\n"
            "安装需要管理员权限，Deepin 将弹出授权对话框。\n"
            "安装后任务栏将自动重启。",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (btn == QMessageBox::Yes) {
            installUpdate();
        }
    });
}

void PanonPlugin::installUpdate()
{
    QString path = QDir::tempPath() + "/dde-dock-panon_update.deb";
    QString cmd = QString("pkexec dpkg -i \"%1\"").arg(path);

    auto *installProc = new QProcess(this);
    connect(installProc, &QProcess::finished, this, [this, installProc](int exitCode, QProcess::ExitStatus) {
        installProc->deleteLater();
        if (exitCode == 0) {
            QMessageBox::information(m_widget, "更新成功",
                "Panon 已更新到 v" + m_updateVersion + "，任务栏即将重启。");
            QProcess::startDetached("killall", QStringList{"dde-shell"});
        } else {
            QMessageBox::warning(m_widget, "安装失败",
                "安装失败，请手动下载安装:\n" + m_updateChecker->downloadUrl());
        }
    });

    installProc->start("/bin/sh", QStringList{"-c", cmd});
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
        double t = (double)i / barCount;
        val *= 0.3 + 12.7 * t * t;
        val = std::sqrt(std::clamp(val * 100.0, 0.0, 1.0));
        if (val < 0.01) val = 0.01;
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
    const int barCount = 27;

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
