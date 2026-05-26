#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QTimer>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(const QString &currentVersion, QObject *parent = nullptr);
    void check();

    QString latestVersion() const { return m_latestVersion; }
    bool hasUpdate() const { return m_hasUpdate; }
    bool isChecking() const { return m_checking; }
    QString downloadUrl() const;

signals:
    void updateAvailable(const QString &latestVersion);
    void upToDate();
    void networkError(const QString &msg);

private slots:
    void onReplyFinished();
    void retry();

private:
    QString m_currentVersion;
    QString m_latestVersion;
    bool m_hasUpdate = false;
    bool m_checking = false;
    QNetworkAccessManager *m_nam = nullptr;
    QTimer *m_retryTimer = nullptr;
    int m_retryCount = 0;
    static constexpr int MAX_RETRIES = 2;
};

#endif
