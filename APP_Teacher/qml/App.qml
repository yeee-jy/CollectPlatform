import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import DelegateUI
import TeacherApp 1.0

import "./Controls/"
import "./Pages/"


DelWindow {
    id: app
    minimumWidth: 1000
    minimumHeight: 800
    title: Qt.application.displayName
    captionBar {
        themeButtonVisible: true
        winIconDelegate: Image {
            source: "qrc:/res/icons/app_logo.ico"
        }
    }
    Component.onCompleted: {
        setSpecialEffect(DelWindow.MicaAlt);
    }

    Item {
        id: content
        anchors.left: parent.left
        anchors.top: app.captionBar.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        DelMenu {
            id: menu
            defaultMenuHeight: 30
            defaultMenuWidth: 150
            anchors.left: content.left
            anchors.top: content.top
            anchors.bottom: aboutButton.top
            anchors.topMargin: 3
            compactMode: true
            defaultSelectedKey: ["home"]
            onClickMenu: (deep, key, data) => {
                console.debug("onClickMenu:", deep, key, JSON.stringify(data))
                if (data && data.source) {
                    app.curSource = data.source
                }
            }
            initModel: [
                {
                    key: "home",
                    label: qsTr("主页"),
                    iconSource: DelIcon.HomeOutlined,
                    source: "file:/D:/Workspace/CollectPlatform/Client/APP_Teacher/qml/Pages/HomePage.qml"
                },
                {
                    key: "realtime",
                    label: qsTr("实时采集"),
                    iconSource: DelIcon.FundProjectionScreenOutlined,
                    source: "file:/D:/Workspace/CollectPlatform/Client/APP_Teacher/qml/Pages/RealTimePage.qml"
                },
                {
                    key: "videoreadtime",
                    label: qsTr("实时视频采集"),
                    iconSource: DelIcon.PlayCircleOutlined ,
                    source: "file:/D:/Workspace/CollectPlatform/Client/APP_Teacher/qml/Pages/videoreadtime.qml"
                },
                {
                    key: "offline",
                    label: qsTr("离线分析"),
                    iconSource: DelIcon.DotChartOutlined,
                    source: "file:/D:/Workspace/CollectPlatform/Client/APP_Teacher/qml/Pages/OfflinePage.qml"
                },
                {
                    key: "storage",
                    label: qsTr("存储库"),
                    iconSource: DelIcon.CloudOutlined,
                    source: "file:/D:/Workspace/CollectPlatform/Client/APP_Teacher/qml/Pages/StoragePage.qml"
                }
            ]

        }

        DelIconButton {
            id: aboutButton
            width: menu.width
            height: menu.defaultMenuHeight
            type: DelButton.Type_Text
            text: menu.compactMode ? "" : qsTr("设置")
            colorText: DelTheme.Primary.colorTextBase
            iconSize: menu.defaultMenuIconSize
            iconSource: DelIcon.SettingOutlined
            effectEnabled: false
            anchors.left: menu.left
            anchors.leftMargin: 5
            anchors.right: menu.right
            anchors.rightMargin: 8
            anchors.bottom: statusBar.top
            anchors.bottomMargin: 5

            property string pageSource: "file:/D:/Workspace/CollectPlatform/Client/APP_Teacher/qml/Pages/SettingPage.qml"

            onClicked: {
                app.curSource = pageSource
                menu.setInactive()
            }

            DelToolTip {
                visible: aboutButton.hovered && !aboutButton.text
                text: qsTr("设置")
                position: DelToolTip.Position_Right
            }
        }

        Item {
            id: container
            anchors.left: menu.right
            anchors.right: content.right
            anchors.top: content.top
            anchors.bottom: statusBar.top
            anchors.margins: 8
            anchors.leftMargin: 0
            clip: true

            Rectangle {
                anchors.fill: container
                color: DelTheme.DelCard.colorBg
                border.color: DelTheme.isDark ? DelTheme.DelCard.colorBorderDark : DelTheme.DelCard.colorBorder
                opacity: 0.6
                radius: 10
            }

            Loader {
                id: containerLoader
                anchors.fill: parent

                NumberAnimation on opacity {
                    running: containerLoader.status === Loader.Ready
                    from: 0
                    to: 1
                    duration: DelTheme.Primary.durationSlow
                }

                function loadSource(source) {
                    if (source == "")
                        return

                    // source = String(source).split("?")[0]

                    containerLoader.source = ""
                    containerLoader.source = source + "?v=" + new Date().getTime()
                }

                function reload() {
                    loadSource(app.curSource)
                }
            }
        }

        Item {
            id: statusBar
            anchors.left: content.left
            anchors.right: content.right
            anchors.bottom: content.bottom
            anchors.margins: 5
            height: 25

            RowLayout {
                anchors.fill: parent
                spacing: 5

                MyIconButton {
                    Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
                    iconSource: menu.compactMode ? "qrc:/res/icons/layout-sidebar-left-off.svg" :
                        "qrc:/res/icons/layout-sidebar-left.svg"
                    onClicked: {
                        menu.compactMode = !menu.compactMode
                    }
                }
            }


        }
    }

    property bool autoReload: false
    property string curSource: ""
    onCurSourceChanged: containerLoader.loadSource(curSource)

    QmlWatcher {
        dirPath: "D:/Workspace/CollectPlatform/Client/APP_Teacher/qml"

        onFileChanged: (filePath) => {
            if (app.autoReload && filePath === app.curSource.substring(6)) {
                containerLoader.reload()
            }
        }
    }

    Rectangle {
        id: toolbar
        width: row.width
        height: row.height
        x: app.width / 2 - width / 2
        y: app.height - height - 5
        color: "transparent"
        border.color: "#ff00ff"

        Row {
            id: row
            spacing: 2

            Image {
                source: "qrc:/res/icons/gripper.svg"
                width: DelTheme.DelButton.fontSize
                height: DelTheme.DelButton.fontSize
                anchors.verticalCenter: parent.verticalCenter

                ColorOverlay {
                    anchors.fill: parent
                    source: parent
                    color: DelTheme.Primary.colorTextBase
                }

                MouseArea {
                    anchors.fill: parent
                    property real lastX: 0
                    drag.target: toolbar
                    drag.axis: Drag.XAxis
                    drag.minimumX: 0
                    drag.maximumX: app.width - toolbar.width
                }

            }

            MyIconButton {
                iconSource: "qrc:/res/icons/flame.svg"
                DelToolTip {
                    visible: parent.hovered
                    text: qsTr("自动热重载")
                    position: DelToolTip.Position_Top
                }
                colorBg: app.autoReload ? DelTheme.DelCaptionButton.colorBgActive : DelTheme.DelCaptionButton.colorBg
                colorText: app.autoReload ? DelTheme.DelButton.colorTextActive : DelTheme.DelButton.colorTextDefault
                onClicked: app.autoReload = !app.autoReload
            }

            MyIconButton {
                iconSource: "qrc:/res/icons/refresh.svg"
                onClicked: {
                    containerLoader.reload()
                }
                DelToolTip {
                    visible: parent.hovered
                    text: qsTr("手动刷新当前页面")
                    position: DelToolTip.Position_Top
                }
            }

            MyIconButton {
                iconSource: "qrc:/res/icons/info.svg"
                DelToolTip {
                    visible: parent.hovered
                    text: app.curSource == "" ? qsTr("当前响应页面：（无）") : qsTr("当前响应页面：\n") + app.curSource
                    position: DelToolTip.Position_Top
                }
            }
        }
    }

    DelMessage {
        id: message
        z: 999
        parent: app.captionBar
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.bottom
    }

}