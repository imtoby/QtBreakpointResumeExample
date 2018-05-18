# QtBreakpointResumeExample
English：It is an example which Breakpoint Resume Download in Qt

中文：这是一个使用 Qt 进行 断点续传 下载的例子

show:

![image](https://github.com/imtoby/QtBreakpointResumeExample/blob/master/QtBreakpointResumeExample/media/79c6i-7vpkb.gif)

code:

```cpp
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

some note:
first of all we need get the server file size, so I create a prepare download file, which can help us to know the total size.

so why need the total size， because of some time user may restart download after downloaded successful, we can see the download will start and appand the finished file size.

Other, In the breakpoint download example, we can found the bytesTotal vale that "void QNetworkReply::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)" signal is not the real total size, but the left size which need to download(less than or equal to total size).

一些说明：
首先我们需要想办法解决获取服务中的文件的大小的问题，在例子中我是通过创建一个临时文件的方式来拿到这个文件的总大小的。

为啥，我们非要知道远端文件的大小？这样是为了避免下载完成后之后，用户再次点击开始下载，继续写入已经下载完成的文件，导致文件损坏。

另外, 在断点续传式的下载中，"void QNetworkReply::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)"信号的参数 bytesTotal 的返回值，其实不是真正的远端文件的大小，而是我们本次要下载的总大小（它小于等于总大小）。
