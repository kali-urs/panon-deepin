#ifndef AUDIOSOURCE_H
#define AUDIOSOURCE_H

#include <QObject>
#include <QThread>
#include <QVector>
#include <QAtomicInt>
#include <QProcess>
#include <cstdint>

class AudioSource : public QThread
{
    Q_OBJECT
public:
    explicit AudioSource(QObject *parent = nullptr);
    ~AudioSource() override;

    bool startCapture();
    void stopCapture();
    bool isCapturing() const { return m_running; }

    int sampleRate() const { return m_sampleRate; }
    int bufferSize() const { return m_bufferSize; }

    QString currentSource() const { return m_sourceName; }
    static QStringList listMonitorSources();
    static QStringList listMonitorSourcesWithState();
    bool switchSource(const QString &name);

signals:
    void samplesReady(const QVector<double> &samples);
    void waveformReady(const QVector<double> &waveform);

protected:
    void run() override;

private:
    QString findMonitorSourceName();
    bool startCaptureTool();
    bool m_running = false;
    QAtomicInt m_stopFlag;
    QProcess *m_process = nullptr;
    QString m_sourceName;
    int m_sampleRate = 44100;
    int m_channels = 2;
    int m_bufferSize = 1024;
    int m_sampleSize = 2;
};

#endif
