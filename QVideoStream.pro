contains(QMAKE_CXX, .*aarch64.*) {
    CONFIG += rpi_arm64
}

contains(CONFIG, rpi_arm64) {
    QT_TARGET_PREFIX = $$[QT_SYSROOT]/usr
    exists($$QT_TARGET_PREFIX/mkspecs/modules/qt_lib_multimedia.pri) {
        QT_MODULE_INCLUDE_BASE = $$QT_TARGET_PREFIX/include
        QT_MODULE_LIB_BASE = $$QT_TARGET_PREFIX/lib
        QT_MODULE_BIN_BASE = $$QT_TARGET_PREFIX/bin
        include($$QT_TARGET_PREFIX/mkspecs/modules/qt_lib_multimedia.pri)
    }
}

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

    contains(CONFIG, rpi_arm64) {
        RPI_SYSROOT = $$[QT_SYSROOT]
        isEmpty(RPI_SYSROOT) {
            RPI_SYSROOT = /home/linux/rpi-sdk/sysroot
        }

        message("QVideoStream: Raspberry Pi ARM64 build")
        QMAKE_CFLAGS += --sysroot=$$RPI_SYSROOT
        QMAKE_CXXFLAGS += --sysroot=$$RPI_SYSROOT
        QMAKE_LFLAGS += --sysroot=$$RPI_SYSROOT

        INCLUDEPATH += \
            $$RPI_SYSROOT/usr/include \
            $$RPI_SYSROOT/usr/include/aarch64-linux-gnu

        LIBS += -L$$RPI_SYSROOT/usr/lib/aarch64-linux-gnu
        QMAKE_LFLAGS += -Wl,-rpath,/usr/lib/aarch64-linux-gnu
    } else {
        message("QVideoStream: Linux Desktop build")
    }

    LIBS += \
        -lavformat \
        -lavcodec \
        -lavdevice \
        -lavfilter \
        -lavutil \
        -lswscale \
        -lswresample
}
