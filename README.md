# QtBreakpointResumeExample
English：It is an example which Breakpoint Resume Download in Qt

中文：这是一个使用 Qt 进行断点续传下载的例子

show:

![image](https://github.com/imtoby/QtBreakpointResumeExample/blob/master/QtBreakpointResumeExample/media/79c6i-7vpkb.gif)

code:

```Qt
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
```
