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

QString AudioSource::findMonitorSourceName()
{
    QProcess proc;
    proc.start("pactl", {"list", "sources", "short"});
    proc.waitForFinished(3000);
    QString info = proc.readAllStandardOutput();

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

    if (!runningMonitor.isEmpty())
        return runningMonitor;
    if (!anyMonitor.isEmpty())
        return anyMonitor;

    return "auto_null.monitor";
}

bool AudioSource::startCapture()
{
    if (m_running) return true;

    m_sourceName = findMonitorSourceName();
    qDebug() << "AudioSource: using monitor source:" << m_sourceName;

    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.rate = m_sampleRate;
    ss.channels = m_channels;

    int error;
    m_paSimple = pa_simple_new(
        nullptr,
        "Panon",
        PA_STREAM_RECORD,
        m_sourceName.toUtf8().constData(),
        "audio capture",
        &ss,
        nullptr,
        nullptr,
        &error
    );

    if (!m_paSimple) {
        qWarning() << "AudioSource: pa_simple_new failed:"
                    << pa_strerror(error);
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

void AudioSource::run()
{
    int16_t rawBuf[m_bufferSize * m_channels];
    QVector<double> samples(m_bufferSize);

    while (!m_stopFlag.loadRelaxed()) {
        int error;
        if (pa_simple_read(m_paSimple, rawBuf, sizeof(rawBuf), &error) < 0) {
            qWarning() << "AudioSource: read error:" << pa_strerror(error);
            break;
        }

        for (int i = 0; i < m_bufferSize; ++i) {
            int sum = 0;
            for (int ch = 0; ch < m_channels; ++ch) {
                sum += rawBuf[i * m_channels + ch];
            }
            samples[i] = sum / (double)(m_channels * 32768);
        }

        emit samplesReady(samples);
        emit waveformReady(samples);
    }

    m_running = false;
}
