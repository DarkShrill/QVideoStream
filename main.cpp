#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "qvideostream.h"

int main(int argc, char *argv[]) {

    QGuiApplication app(argc, argv);

    QVideoStream::registerTypes();

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
