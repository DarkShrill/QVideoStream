#include "qvideostream.h"
#include "videorendereritem.h"

#include <qqml.h>

void QVideoStream::registerTypes()
{
    static bool registered = false;

    if (registered) {
        return;
    }

    registered = true;

    qmlRegisterType<VideoRendererItem>(
        "VideoStream",
        1,
        0,
        "VideoStream"
        );
}
