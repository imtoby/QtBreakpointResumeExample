import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import com.toby.examples.downloadmanager 1.0

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("Qt Breakpoint Resume Download Example")

    DownloadManager {
        id: downloadManager
        onStatusChanged: {
            var percent = bytesReceived * 1.0 / bytesTotal;
            progressBar.value = percent;
            progressText.text = parseInt(bytesReceived * 100.0 / bytesTotal) + "%";
        }
        onSpeedStringChanged: {
            speedText.text = speedString
        }
    }

    Item {
        anchors.fill: parent

        RowLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 20
            anchors.top: parent.top

            TextField {
                id: textUrl
                placeholderText: qsTr("Please Enter Url")
            }

            Button {
                id: startBtn
                text: qsTr("Start")
                onClicked: {
                    if (textUrl.text.length == 0) {
                        downloadManager.start("https://download.virtualbox.org/virtualbox/5.2.12/virtualbox-5.2_5.2.12-122591~Ubuntu~trusty_amd64.deb");
                    } else {
                        downloadManager.start(textUrl.text);
                    }
                }
            }

            Button {
                id: pauseBtn
                text: qsTr("Pause")
                onClicked: {
                    downloadManager.pause();
                }
            }
        }

        RowLayout {
            anchors.centerIn: parent
            ProgressBar {
                id: progressBar
                value: 0.0
            }
            Text {
                id: progressText
            }
            Text {
                id: speedText
            }
        }
    }
}
