#include "audiosource.h"
#include <QDebug>
#include <QProcess>
#include <QFileInfo>
#include <QFile>
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

QString AudioSource::findMonitorSourceName()
{
    QProcess proc;
    proc.start("pactl", {"list", "sources", "short"});
    proc.waitForFinished(3000);
    QString info = proc.readAllStandardOutput();

    qDebug() << "AudioSource: all sources output:" << info.trimmed();

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

    qWarning() << "AudioSource: no .monitor source found, trying default-sink monitor";

    QProcess proc2;
    proc2.start("pactl", {"info"});
    proc2.waitForFinished(3000);
    QString info2 = proc2.readAllStandardOutput();
    qDebug() << "AudioSource: pactl info output:" << info2.trimmed();
    for (const QString &line : info2.split('\n')) {
        if (line.startsWith("Default Sink:")) {
            QString defaultSink = line.mid(QString("Default Sink:").length()).trimmed();
            if (!defaultSink.isEmpty()) {
                QString monitorName = defaultSink + ".monitor";
                qDebug() << "AudioSource: trying default sink monitor:" << monitorName;
                return monitorName;
            }
        }
    }

    qWarning() << "AudioSource: falling back to auto_null.monitor";
    return "auto_null.monitor";
}

static bool canExec(const QString &name)
{
    return QFileInfo::exists("/usr/bin/" + name)
        || QFileInfo::exists("/bin/" + name);
}

static QByteArray readAll(QProcess *p, int bytes)
{
    while (p->bytesAvailable() < bytes) {
        if (p->state() != QProcess::Running)
            return {};
        if (!p->waitForReadyRead(100))
            return {};
    }
    return p->read(bytes);
}

static QString sinkNameFromMonitor(const QString &monitorName)
{
    if (monitorName.endsWith(".monitor"))
        return monitorName.left(monitorName.size() - 8);
    return monitorName;
}

bool AudioSource::startCapture()
{
    if (m_running) return true;

    m_sourceName = findMonitorSourceName();
    qDebug() << "AudioSource: using source:" << m_sourceName;

    m_stopFlag.storeRelaxed(0);
    m_running = true;
    start();
    return true;
}

void AudioSource::stopCapture()
{
    m_stopFlag.storeRelaxed(1);

    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
        delete m_process;
        m_process = nullptr;
    }

    m_running = false;

    if (isRunning()) {
        quit();
        wait(2000);
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
    const int frameBytes = m_bufferSize * m_channels * m_sampleSize;

    // Use pw-record (native PipeWire) — avoids PulseAudio compat layer which breaks audio
    if (!canExec("pw-record")) {
        qWarning() << "AudioSource: pw-record not found, install pipewire-bin";
        m_running = false;
        return;
    }

    QString target = sinkNameFromMonitor(m_sourceName);
    QStringList args = {"--format=s16", "--rate=44100", "--channels=2",
                        "--latency=20msec", "--target=" + target, "/dev/stdout"};

    qDebug() << "AudioSource: starting pw-record with args:" << args;

    m_process = new QProcess;
    m_process->start("pw-record", args);
    if (!m_process->waitForStarted(5000)) {
        qWarning() << "AudioSource: pw-record failed to start, trying without --target";
        // Retry without target (use default source)
        args.removeAt(3);
        m_process->start("pw-record", args);
        if (!m_process->waitForStarted(5000)) {
            qWarning() << "AudioSource: pw-record failed:" << m_process->errorString();
            m_running = false;
            return;
        }
    }

    // pw-record writes a 44-byte WAV header; skip it
    QByteArray header = readAll(m_process, 44);
    if (header.size() < 44)
        qWarning() << "AudioSource: short WAV header:" << header.size();

    QVector<double> samples(m_bufferSize);

    while (!m_stopFlag.loadRelaxed()) {
        if (m_process->state() != QProcess::Running) {
            qWarning() << "AudioSource: pw-record exited";
            break;
        }

        QByteArray data = readAll(m_process, frameBytes);
        if (data.size() < frameBytes) {
            if (data.isEmpty()) continue;
            data.resize(frameBytes);
        }

        const int16_t *buf = reinterpret_cast<const int16_t *>(data.constData());
        for (int i = 0; i < m_bufferSize; ++i) {
            int sum = 0;
            for (int ch = 0; ch < m_channels; ++ch) {
                sum += buf[i * m_channels + ch];
            }
            samples[i] = sum / (double)(m_channels * 32768);
        }

        emit samplesReady(samples);
        emit waveformReady(samples);
    }

    m_process->kill();
    m_process->waitForFinished(1000);

    qDebug() << "AudioSource: capture stopped";
    m_running = false;
}
