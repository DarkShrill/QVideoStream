QT      += quick
CONFIG  += c++11
QT      += qml
QT      += network
QT      += quickcontrols2
QT      += widgets
QT      += core gui
QT      += multimedia

TEMPLATE = lib # app

CONFIG += c++17

SOURCES += \
    main.cpp \
    videorendereritem.cpp \
    videodecoder.cpp

HEADERS += \
    videorendereritem.h \
    videodecoder.h

RESOURCES += qml.qrc

INCLUDEPATH += /usr/include \
               /usr/local/include

win32{
INCLUDEPATH += $$PWD/ffmpeg/include
LIBS += $$PWD/ffmpeg/lib/avcodec.lib \
        $$PWD/ffmpeg/lib/avdevice.lib \
        $$PWD/ffmpeg/lib/avfilter.lib \
        $$PWD/ffmpeg/lib/avformat.lib \
        $$PWD/ffmpeg/lib/avutil.lib \
        $$PWD/ffmpeg/lib/postproc.lib \
        $$PWD/ffmpeg/lib/swresample.lib \
        $$PWD/ffmpeg/lib/swscale.lib
message($$PWD/ffmpeg/lib)
}
