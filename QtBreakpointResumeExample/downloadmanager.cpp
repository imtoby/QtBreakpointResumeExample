#include "downloadmanager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QFile>
#include <QList>
#include <QTimer>
#include <QFileInfo>
#include <QDebug>
#include <QPair>
#include <QNetworkConfiguration>

namespace {
const QString TAG = "DownloadManager:";
}

class DownloadManagerPrivate
{
public:
    DownloadManagerPrivate(DownloadManager* parent)
        : q_ptr(parent)
        , prepareDownload(0)
        , currentDownload(0)
        , downloadedBytes(0)
        , totalBytes(-1)
    {}

private:
    DownloadManager * q_ptr;
    Q_DECLARE_PUBLIC(DownloadManager)

    QNetworkAccessManager manager;
    QList<QUrl> downloadQueue;
    QNetworkReply *prepareDownload;
    QNetworkReply *currentDownload;
    QFile prepareOutput;
    QFile output;
    QTime downloadTime;

    qint64 downloadedBytes;
    qint64 totalBytes;
};

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new DownloadManagerPrivate(this))
{
}

DownloadManager::~DownloadManager()
{
}

void DownloadManager::start(const QUrl &url)
{
    Q_D(DownloadManager);
    if (d->downloadQueue.isEmpty())
        QTimer::singleShot(0, this, SLOT(startNextDownload()));

    d->downloadQueue.push_back(url);
}

void DownloadManager::start(const QStringList &urlList)
{
    Q_D(DownloadManager);
    foreach (QString url, urlList)
        start(QUrl::fromEncoded(url.toLocal8Bit()));

    if (d->downloadQueue.isEmpty())
        QTimer::singleShot(0, this, SIGNAL(finished()));
}

QString DownloadManager::saveFileName(const QUrl &url)
{
    QString path = url.path();
    QFileInfo fileInfo = QFileInfo(path);
    QString basename = QFileInfo(path).fileName();

    if (basename.isEmpty())
        basename = "download";

    if (QFile::exists(basename)) {
        return basename;
    }

    return basename;
}

void DownloadManager::pause()
{
    Q_D(DownloadManager);
    if (NULL != d->currentDownload && d->currentDownload->isRunning()) {
        d->currentDownload->abort();
        emit speedStringChanged("0 bytes/s");
    }
}

void DownloadManager::startNextDownload()
{
    Q_D(DownloadManager);
    if (d->downloadQueue.isEmpty()) {
        emit finished();
        return;
    }

    QUrl url = d->downloadQueue.at(0);
    QString filename = saveFileName(url);

    d->prepareOutput.setFileName(filename + ".prepare");

    if (!d->prepareOutput.open(QIODevice::WriteOnly)) {
        qDebug("Problem opening save file '%s' for download '%s': %s\n",
                qPrintable(filename + ".prepare"), url.toEncoded().constData(),
                qPrintable(d->prepareOutput.errorString()));
        return;
    }
    QNetworkRequest request(url);

    QNetworkConfiguration config = d->manager.configuration();
    config.setConnectTimeout(10000); // 10 secends
    d->manager.setConfiguration(config);

    d->prepareDownload = d->manager.get(request);

    connect(d->prepareDownload, SIGNAL(readyRead()),
            SLOT(prepareReadyRead()));

}

void DownloadManager::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_D(DownloadManager);
    // calculate the download speed
    double speed = bytesReceived * 1000.0 / d->downloadTime.elapsed();
    QString unit;
    if (speed < 1024) {
        unit = "bytes/s";
    } else if (speed < 1024*1024) {
        speed /= 1024;
        unit = "kB/s";
    } else {
        speed /= 1024*1024;
        unit = "MB/s";
    }

    const qint64 currentSize = d->output.size();
    emit statusChanged(currentSize, d->downloadedBytes + bytesTotal, speed);
    emit speedStringChanged(QString::fromLatin1("%1 %2")
                            .arg(speed, 3, 'f', 1).arg(unit));
}

void DownloadManager::downloadFinished()
{
    Q_D(DownloadManager);
    d->output.close();
    d->totalBytes = -1;

    if (d->currentDownload->error()) {
        // download failed
        qDebug("Failed: %s\n", qPrintable(d->currentDownload->errorString()));
    } else {
        qDebug("Succeeded.\n");
    }

    emit speedStringChanged("");

    d->currentDownload->deleteLater();
    startNextDownload();
}

void DownloadManager::downloadReadyRead()
{
    Q_D(DownloadManager);
    if (d->downloadedBytes == d->totalBytes) {
        qDebug() << TAG << "downloadedBytes == contentLength abort...";
        d->currentDownload->abort();
        downloadFinished();
        return;
    }
    d->output.write(d->currentDownload->readAll());
}

void DownloadManager::prepareReadyRead()
{
    Q_D(DownloadManager);
    if (d->downloadQueue.isEmpty()) {
        emit finished();
        return;
    }

    d->totalBytes =
            d->prepareDownload->rawHeader("Content-Length").toLongLong();

    if (d->totalBytes > 0) {
        d->prepareDownload->abort();
        d->prepareOutput.remove();
        d->prepareOutput.close();
    } else {
        qDebug() << "error";
        startNextDownload();
        return;
    }

    qDebug() << TAG << "total size: " << d->totalBytes;

    QUrl url = d->downloadQueue.takeFirst();
    QString filename = saveFileName(url);

    d->output.setFileName(filename);
    d->downloadedBytes = d->output.size();

    qDebug() << TAG << "current size: " << d->downloadedBytes;

    if (d->downloadedBytes == d->totalBytes) {
        emit statusChanged(d->downloadedBytes, d->totalBytes, 0);
        emit speedStringChanged("");
        d->totalBytes = -1;
        startNextDownload();
        return;
    }

    if (!d->output.open(QIODevice::Append)) {
        qDebug("Problem opening save file '%s' for download '%s': %s\n",
                qPrintable(filename), url.toEncoded().constData(),
                qPrintable(d->output.errorString()));

        startNextDownload();
        return;                 // skip this download
    }

    QNetworkRequest request(url);
    request.setRawHeader(QByteArray("Range"),
                         QString("bytes=%1-").arg(d->downloadedBytes).toLocal8Bit());

    QNetworkConfiguration config = d->manager.configuration();
    config.setConnectTimeout(10000); // 10 secends
    d->manager.setConfiguration(config);

    d->currentDownload = d->manager.get(request);
    connect(d->currentDownload, SIGNAL(downloadProgress(qint64, qint64)),
            SLOT(downloadProgress(qint64, qint64)));
    connect(d->currentDownload, SIGNAL(finished()),
            SLOT(downloadFinished()));
    connect(d->currentDownload, SIGNAL(readyRead()),
            SLOT(downloadReadyRead()));
    connect(d->currentDownload, SIGNAL(error(QNetworkReply::NetworkError)),
            SLOT(downloadError(QNetworkReply::NetworkError)));

    // prepare the output
    qDebug("Downloading %s...\n", url.toEncoded().constData());
    d->downloadTime.start();
}

void DownloadManager::downloadError(QNetworkReply::NetworkError code)
{
    qDebug() << "DownloadManager::downloadError code: " << code;
}
