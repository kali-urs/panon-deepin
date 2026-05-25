#ifndef PANONPLUGIN_H
#define PANONPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include <pluginsiteminterface.h>

class SpectrumWidget;
class AudioSource;
class FFTProcessor;

class PanonPlugin : public QObject, PluginsItemInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterface)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "panon.json")

public:
    explicit PanonPlugin(QObject *parent = nullptr);
    ~PanonPlugin() override;

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;
    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    QWidget *itemPopupApplet(const QString &itemKey) override;
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) override;
    void positionChanged(const Dock::Position position) override;

    PluginType type() override { return Normal; }
    PluginSizePolicy pluginSizePolicy() const override { return System; }

private:
    void onSamplesReady(const QVector<double> &samples);
    void onStereoReady(const QVector<double> &left, const QVector<double> &right);
    void onWaveformReady(const QVector<double> &waveform);
    void updateOrientation();
    void setWidth(int w);
    void reapplyWidth();

    SpectrumWidget *m_widget = nullptr;
    AudioSource *m_audioSource = nullptr;
    FFTProcessor *m_fft = nullptr;
    PluginProxyInterface *m_proxyInter = nullptr;
    bool m_paused = false;
    int m_width = 150;
};

#endif
