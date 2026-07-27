QT += core gui quick qml network quickcontrols2 widgets multimedia

CONFIG += c++17
CONFIG -= app_bundle

TEMPLATE = lib
TARGET = QVideoStream

DEFINES += QVIDEOSTREAM_LIBRARY

CONFIG(debug, debug|release) {
    DESTDIR = $$OUT_PWD/debug
} else {
    DESTDIR = $$OUT_PWD/release
}

SOURCES += \
    qvideostream.cpp \
    videorendereritem.cpp \
    videodecoder.cpp

HEADERS += \
    qvideostream.h \
    qvideostream_global.h \
    videorendereritem.h \
    videodecoder.h

RESOURCES += qml.qrc

win32 {
    CONFIG += dll

    INCLUDEPATH += $$PWD/ffmpeg/include

    LIBS += \
        $$PWD/ffmpeg/lib/avcodec.lib \
        $$PWD/ffmpeg/lib/avdevice.lib \
        $$PWD/ffmpeg/lib/avfilter.lib \
        $$PWD/ffmpeg/lib/avformat.lib \
        $$PWD/ffmpeg/lib/avutil.lib \
        $$PWD/ffmpeg/lib/postproc.lib \
        $$PWD/ffmpeg/lib/swresample.lib \
        $$PWD/ffmpeg/lib/swscale.lib
}

unix:!macx {
    CONFIG += shared

    QMAKE_CFLAGS += --sysroot=/home/linux/rpi-sdk/sysroot
    QMAKE_CXXFLAGS += --sysroot=/home/linux/rpi-sdk/sysroot
    QMAKE_LFLAGS += --sysroot=/home/linux/rpi-sdk/sysroot

    INCLUDEPATH += \
        /home/linux/rpi-sdk/sysroot/usr/include \
        /home/linux/rpi-sdk/sysroot/usr/include/aarch64-linux-gnu

    LIBS += \
        -L/home/linux/rpi-sdk/sysroot/usr/lib/aarch64-linux-gnu \
        -lavformat \
        -lavcodec \
        -lavdevice \
        -lavfilter \
        -lavutil \
        -lswscale \
        -lswresample

    QMAKE_LFLAGS += -Wl,-rpath,/usr/lib/aarch64-linux-gnu
}
