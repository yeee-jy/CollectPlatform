import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtGraphs
import DelegateUI

import TeacherApp 1.0
import "../Controls/"
import "../Controls/Charts/"
import "../Views/"
import "../../src/rtmp"
import VideoComponents 1.0

MyPage {
    id: page
    titleIconSource: DelIcon.PlayCircleOutlined
    titleText: qsTr("实时视频数据采集与分析")
    spacing: 20

    Column {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 50

        Rectangle {
            id: videoContainer
            width: 640
            height: 480
            color: "black"

            VideoPaintedItem {
                id: videoDisplay
                anchors.fill: parent
            }

            Connections {
                target: pull_work
                onImageReady: {
                    videoDisplay.updateFrame(image)
                }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 40

            Button {
                id: playButton
                checkable: true
                width: 120
                height: 40

                text: playButton.checked ? "暂停" : "开始播放"

                background: Rectangle {
                    radius: 8
                    color: playButton.checked ? "#4a86e8" : "#5d9cec"
                    border.color: "#4a86e8"
                    border.width: 1
                }

                contentItem: Text {
                    text: playButton.text
                    font { pixelSize: 16; bold: true }
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    console.log("播放按钮状态:", playButton.checked ? "已选中" : "未选中")
                }
            }

            Button {
                id: startButton
                checkable: false
                width: 120
                height: 40
                enabled: !startButton.isInitialized

                property bool isInitializing: false
                property bool isInitialized: false

                text: startButton.isInitializing ? "初始化中..." :
                    startButton.isInitialized ? "初始化完成" : "开始初始化"

                background: Rectangle {
                    radius: 8
                    color: startButton.isInitializing ? "#ff9800" :
                        startButton.isInitialized ? "#4caf50" : "#5d9cec"
                    border.color: "#4a86e8"
                    border.width: 1
                }

                contentItem: Text {
                    text: startButton.text
                    font { pixelSize: 16; bold: true }
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (!startButton.isInitializing && !startButton.isInitialized) {
                        startButton.isInitializing = true;
                        Qt.callLater(function() {
                            pull_work.isTrueGet();
                            startButton.isInitializing = false;
                            startButton.isInitialized = true;
                        });
                    }
                }
            }
        }
    }
}