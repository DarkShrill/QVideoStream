#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "videorendereritem.h"

int main(int argc, char *argv[]) {

    QGuiApplication app(argc, argv);

    qmlRegisterType<VideoRendererItem>("VideoStream", 1, 0, "VideoStream");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
