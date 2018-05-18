#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QNetworkReply>

class DownloadManagerPrivate;
class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = 0);
    ~DownloadManager();

public slots:
    void start(const QUrl &url);
    void start(const QStringList &urlList);
    QString saveFileName(const QUrl &url);
    void pause();

signals:
    void finished();
    void statusChanged(qint64 bytesReceived, qint64 bytesTotal, double speed);
    void speedStringChanged(QString speedString);

private slots:
    void startNextDownload();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished();
    void downloadReadyRead();
    void prepareReadyRead();
    void downloadError(QNetworkReply::NetworkError code);

private:
    QScopedPointer<DownloadManagerPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DownloadManager)
};

#endif // DOWNLOADMANAGER_H
