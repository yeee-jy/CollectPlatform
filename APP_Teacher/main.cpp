#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QFileSystemWatcher>
#include <QTimer>
#include "delapp.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <qqmlcontext.h>
#include "WristbandData.h"
#include "src/rtmp/PullWork.h"
#include "src/rtmp/VideoPaintedItem.h"
#include <QtQml/QQmlEngine>

int main(int argc, char* argv[])
{
    //使用cpu渲染
    qmlRegisterType<VideoPaintedItem>("VideoComponents", 1, 0, "VideoPaintedItem");

    //使用GPU加速
    // qmlRegisterType<VideoFboItem>("VideoComponents", 1, 0, "VideoFboItem");
    // QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/res/icons/app_logo.ico"));
    app.setApplicationDisplayName(QObject::tr("多模态数据采集与学习者状态实时感知平台"));
    app.setApplicationVersion("1.0.0"); //版本号
    app.setOrganizationName("perlab");//设置组织名称和域名
    app.setOrganizationDomain("nerc-ebd.ccnu.edu.cn");

    QQmlApplicationEngine engine;
    DelApp::initialize(&engine);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QGuiApplication::exit(-1); },
        Qt::QueuedConnection
    );
    // engine.loadFromModule("qml", "App");

    //创建拉流的类，注入到调用界面
    PullWork pull_work;
    engine.rootContext()->setContextProperty("pull_work",&pull_work);

    const QString qmlPath = "D:/Workspace/CollectPlatform/Client/APP_Teacher/qml/";
    const QUrl mainQmlUrl = QUrl::fromLocalFile(qmlPath + "App.qml");
    engine.load(mainQmlUrl);

    // WristBandData data {
    //     QDateTime::currentMSecsSinceEpoch(),
    //     { { 1, 1 }, { 2, 2 }, { 3, 3 } },
    //     0,
    //     { 1, 2, 3 },
    //     { { 1, 1, 1 }, { 2, 2, 2 }, { 3, 3, 3 } }
    // };

    // qDebug() << data;

    // QJsonDocument doc = QJsonDocument::fromJson(R"(
    //     {
    //         "timestamp": 1672531199000,
    //         "ppg": 75.5,
    //         "pulseWaveDatas": [
    //             { "rawVal": 1.0, "filteredVal": 0.5 },
    //             { "rawVal": 2.0, "filteredVal": 1.5 }
    //         ],
    //         "gsrs": [0.1, 0.2, 0.3],
    //         "accDatas": [
    //             { "x": 1.0, "y": 2.0, "z": 3.0 },
    //             { "x": 4.0, "y": 5.0, "z": 6.0 }
    //         ]
    //     }
    // )");
    // WristBandData data = WristBandData::fromJsonObject(doc.object());
    //
    // qRegisterMetaType<WristBandData>("WristBandData");

    return app.exec();
}
