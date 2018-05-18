#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <downloadmanager.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterType<DownloadManager>("com.toby.examples.downloadmanager", 1, 0, "DownloadManager");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
