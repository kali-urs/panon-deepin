#include "audiosource.h"
#include <QDebug>
#include <QProcess>
#include <cmath>

AudioSource::AudioSource(QObject *parent)
    : QThread(parent)
{
}

AudioSource::~AudioSource()
{
    stopCapture();
}

QStringList AudioSource::listMonitorSources()
{
    QStringList result;
    QProcess proc;
    proc.start("pactl", {"list", "sources", "short"});
    proc.waitForFinished(3000);
    QString info = proc.readAllStandardOutput();

    for (const QString &line : info.split('\n')) {
        QStringList cols = line.split('\t');
        if (cols.size() < 5) continue;
        QString name = cols[1].trimmed();
        if (!name.endsWith(".monitor")) continue;
        result << name;
    }
    return result;
}

QStringList AudioSource::listMonitorSourcesWithState()
{
    QStringList result;
    QProcess proc;
    proc.start("pactl", {"list", "sources", "short"});
    proc.waitForFinished(3000);
    QString info = proc.readAllStandardOutput();

    for (const QString &line : info.split('\n')) {
        QStringList cols = line.split('\t');
        if (cols.size() < 5) continue;
        QString name = cols[1].trimmed();
        if (!name.endsWith(".monitor")) continue;
        QString state = cols[4].trimmed();
        result << name + "\t" + state;
    }
    return result;
}

void AudioSource::printAudioDiagnostics()
{
    QProcess proc;

    proc.start("pactl", {"info"});
    proc.waitForFinished(3000);
    qDebug() << "=== pactl info ===" << proc.readAllStandardOutput().trimmed();

    proc.start("pactl", {"list", "sources"});
    proc.waitForFinished(3000);
    qDebug() << "=== pactl list sources ===" << proc.readAllStandardOutput().trimmed();

    proc.start("pactl", {"list", "sinks"});
    proc.waitForFinished(3000);
    qDebug() << "=== pactl list sinks ===" << proc.readAllStandardOutput().trimmed();
}

QString AudioSource::findMonitorSourceName()
{
    QProcess proc;
    proc.start("pactl", {"list", "sources", "short"});
    proc.waitForFinished(3000);
    QString info = proc.readAllStandardOutput();

    qDebug() << "=== pactl list sources short ===" << info.trimmed();

    QString runningMonitor;
    QString anyMonitor;

    for (const QString &line : info.split('\n')) {
        QStringList cols = line.split('\t');
        if (cols.size() < 5) continue;
        QString name = cols[1].trimmed();
        QString state = cols[4].trimmed();
        if (!name.endsWith(".monitor")) continue;
        anyMonitor = name;
        if (state == "RUNNING") {
            runningMonitor = name;
        }
    }

    if (!runningMonitor.isEmpty()) {
        qDebug() << "AudioSource: found RUNNING monitor:" << runningMonitor;
        return runningMonitor;
    }
    if (!anyMonitor.isEmpty()) {
        qDebug() << "AudioSource: no RUNNING monitor, using:" << anyMonitor;
        return anyMonitor;
    }

    // Fallback: construct from default sink
    proc.start("pactl", {"info"});
    proc.waitForFinished(3000);
    for (const QString &line : proc.readAllStandardOutput().split('\n')) {
        if (line.startsWith("Default Sink:")) {
            QString sink = line.mid(13).trimmed();
            if (!sink.isEmpty()) {
                QString monitor = sink + ".monitor";
                qDebug() << "AudioSource: constructed from default sink:" << monitor;
                return monitor;
            }
        }
    }

    qWarning() << "AudioSource: falling back to auto_null.monitor";
    return "auto_null.monitor";
}

bool AudioSource::startCapture()
{
    if (m_running) return true;

    printAudioDiagnostics();

    m_sourceName = findMonitorSourceName();
    qDebug() << "AudioSource: using monitor source:" << m_sourceName;

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.rate = m_sampleRate;
    ss.channels = m_channels;

    int error;

    int fragSize = m_bufferSize * m_channels * sizeof(int16_t);
    pa_buffer_attr attr;
    attr.maxlength = (uint32_t)-1;
    attr.tlength   = (uint32_t)-1;
    attr.prebuf    = (uint32_t)-1;
    attr.minreq    = (uint32_t)-1;
    attr.fragsize  = fragSize;

    m_paSimple = pa_simple_new(
        nullptr,
        "Panon",
        PA_STREAM_RECORD,
        m_sourceName.toUtf8().constData(),
        "audio capture",
        &ss,
        nullptr,
        &attr,
        &error
    );

    if (!m_paSimple) {
        qWarning() << "AudioSource: pa_simple_new failed:" << pa_strerror(error);
        return false;
    }

    m_stopFlag.storeRelaxed(0);
    m_running = true;
    start();
    return true;
}

void AudioSource::stopCapture()
{
    m_stopFlag.storeRelaxed(1);
    m_running = false;

    if (isRunning()) {
        quit();
        wait(2000);
    }

    if (m_paSimple) {
        pa_simple_free(m_paSimple);
        m_paSimple = nullptr;
    }
}

bool AudioSource::switchSource(const QString &name)
{
    stopCapture();
    m_sourceName = name;
    return startCapture();
}

void AudioSource::run()
{
    int16_t rawBuf[m_bufferSize * m_channels];
    QVector<double> mono(m_bufferSize);
    QVector<double> left(m_bufferSize);
    QVector<double> right(m_bufferSize);

    qDebug() << "AudioSource: capture thread started, buffer:" << m_bufferSize
             << "rate:" << m_sampleRate << "channels:" << m_channels;

    while (!m_stopFlag.loadRelaxed()) {
        int error;
        if (pa_simple_read(m_paSimple, rawBuf, sizeof(rawBuf), &error) < 0) {
            qWarning() << "AudioSource: read error:" << pa_strerror(error);
            break;
        }

        for (int i = 0; i < m_bufferSize; ++i) {
            left[i]  = rawBuf[i * m_channels]     / 32768.0;
            right[i] = rawBuf[i * m_channels + 1] / 32768.0;
            mono[i]  = (left[i] + right[i]) * 0.5;
        }

        emit samplesReady(mono);
        emit stereoReady(left, right);
        emit waveformReady(mono);
    }

    qDebug() << "AudioSource: capture thread stopped";
    m_running = false;
}
