#include "updatechecker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>
#include <QDebug>

UpdateChecker::UpdateChecker(const QString &currentVersion, QObject *parent)
    : QObject(parent)
    , m_currentVersion(currentVersion)
{
}

void UpdateChecker::check()
{
    if (m_checking) return;
    m_checking = true;

    if (m_proc) {
        m_proc->deleteLater();
        m_proc = nullptr;
    }

    m_proc = new QProcess(this);
    connect(m_proc, &QProcess::finished, this, &UpdateChecker::onProcessFinished);

    QString url("https://api.github.com/repos/kali-urs/panon-deepin/releases/latest");
    m_proc->start("curl", QStringList{"-s", url});
}

void UpdateChecker::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_checking = false;

    if (exitCode != 0 || status != QProcess::NormalExit) {
        if (m_retryCount < MAX_RETRIES) {
            m_retryCount++;
            if (!m_retryTimer) {
                m_retryTimer = new QTimer(this);
                m_retryTimer->setSingleShot(true);
                connect(m_retryTimer, &QTimer::timeout, this, &UpdateChecker::retry);
            }
            m_retryTimer->start(30000);
        } else {
            emit networkError("curl failed");
        }
        return;
    }

    m_retryCount = 0;

    QByteArray data;
    if (m_proc) {
        data = m_proc->readAllStandardOutput();
        m_proc->deleteLater();
        m_proc = nullptr;
    }
    parseResponse(data);
}

void UpdateChecker::parseResponse(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit networkError("invalid JSON response");
        return;
    }

    QJsonObject obj = doc.object();
    QString tagName = obj["tag_name"].toString();
    if (tagName.isEmpty()) {
        emit networkError("missing tag_name in response");
        return;
    }

    QString latest = tagName;
    if (latest.startsWith('v'))
        latest = latest.mid(1);

    m_latestVersion = latest;

    QVersionNumber current = QVersionNumber::fromString(m_currentVersion);
    QVersionNumber remote = QVersionNumber::fromString(latest);

    if (!remote.isNull() && remote > current) {
        m_hasUpdate = true;
        emit updateAvailable(latest);
        qDebug() << "UpdateChecker: update available:" << latest
                 << "(current:" << m_currentVersion << ")";
    } else {
        m_hasUpdate = false;
        emit upToDate();
    }
}

void UpdateChecker::retry()
{
    check();
}

QString UpdateChecker::downloadUrl() const
{
    return QString("https://github.com/kali-urs/panon-deepin/releases/download/v%1/dde-dock-panon_%1-1_amd64.deb")
        .arg(m_latestVersion);
}
