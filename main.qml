import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import VideoStream 1.0

ApplicationWindow {
    visible: true
    width: 1280
    height: 720
    id:window
    title: "QVideoStream"


    property var videoUrls: [
        // "video=Full HD webcam"
        // "rtsp://55.44.33.22:554"
    ]


    property int columnCount: Math.min(3, videoUrls.length)

    Column {
        anchors.fill: parent

        Grid {
            id: videoGrid
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 10
            columns: getColumnCount(videoUrls.length)

            Repeater {
                model: videoUrls.length

                VideoStream {
                    width: videoGrid.width / getColumnCount(videoUrls.length)
                    height: width * 0.5625

                    Component.onCompleted: start(videoUrls[index])
                }
            }
        }

        Row {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter

            TextField {
                id: urlInput
                placeholderText: "Add Url"
                width: 800
            }

            Button {
                text: "Add URL"
                onClicked: {
                    if (urlInput.text.length > 0)
                        videoUrls = [...videoUrls, urlInput.text]

                    console.log(videoUrls)
                }
            }

            Button {
                text: "Clear All"
                onClicked: videoUrls = []
            }
        }
    }


    function getColumnCount(count) {
        return Math.min(3, count);
    }
}
