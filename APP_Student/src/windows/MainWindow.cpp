#include "MainWindow.h"

#include <QCloseEvent>
#include <QSplitter>

#include "services/EEGDataReceiver.h"
#include "services/WristbandServer.h"
#include "views/CameraView.h"
#include "views/SettingView.h"
#include "views/LogView.h"
#include "./views/CameraView.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_eegReceiver(new EEGDataReceiver), m_bandServer(new WristbandServer) {
    initUI();
    initCamera();
    initConnection();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_eegReceiver->stop();
    m_eegThread.quit();
    m_eegThread.wait();
    m_eegReceiver->deleteLater();
    m_bandServer->stop();
    m_bandServer->deleteLater();
    event->accept();
}

void MainWindow::initUI() {
    setWindowTitle("MQTT Client");

    ui_settingView = new SettingView;
    ui_settingView->setMaximumWidth(400);
    ui_eegView = new LogView(tr("EEG"), ":/res/icons/head-snowflake-outline.svg");
    ui_bandView = new LogView(tr("Wristband"), ":/res/icons/watch.svg");

    // ui_cameraView = new CameraView;
    ui_videowidget = new VideoWidget;

    auto view4 = new LogEdit(20);


    view4->setPlaceholderText(tr("Server log will be displayed here..."));
    view4->setMaximumHeight(150);

    auto* contentWidget = new QWidget;
    auto* contentLayout = new QGridLayout(contentWidget);
    contentLayout->addWidget(ui_eegView, 0, 0);
    contentLayout->addWidget(ui_bandView, 0, 1);
    // contentLayout->addWidget(ui_cameraView, 1, 0, 1, 2);
    contentLayout->addWidget(ui_videowidget, 1, 0, 1, 2);


    auto* contentSplitter = new QSplitter(Qt::Vertical);
    contentSplitter->addWidget(contentWidget);
    contentSplitter->addWidget(view4);
    contentSplitter->setCollapsible(1, false);

    auto* mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(ui_settingView);
    mainSplitter->addWidget(contentSplitter);
    mainSplitter->setCollapsible(0, false);

    setCentralWidget(mainSplitter);
}

void MainWindow::initCamera() {
    // ui_cameraView->initCamera(ui_settingView->cameraDevice(), ui_settingView->cameraFormat());
}

void MainWindow::initConnection() {
    /******************** EEG ********************/
    m_eegReceiver->moveToThread(&m_eegThread);
    connect(ui_settingView, &SettingView::requestConnectEEG, this, [=](const QString& address, const int port) {
        ui_eegView->setUIConnecting(tr("Connecting to DSI-Streamer..."));
        m_eegReceiver->start(address, port);
    });
    connect(ui_settingView, &SettingView::requestDisconnectEEG, this, [=] {
        ui_eegView->setUIDisconnecting();
        m_eegReceiver->stop();
    });
    connect(m_eegReceiver, &EEGDataReceiver::connected, this, [=] {
        ui_settingView->onEEGConnected();
        ui_eegView->setUIConnected();
    });
    connect(m_eegReceiver, &EEGDataReceiver::disconnected, this, [=] {
        ui_settingView->onEEGDisconnected();
        ui_eegView->setUIDisconnected();
    });
    connect(m_eegReceiver, &EEGDataReceiver::errorOccurred, this, [=] {
        ui_settingView->onEEGDisconnected();
        ui_eegView->setUIDisconnected();
    });
    connect(m_eegReceiver, &EEGDataReceiver::logFetched, ui_eegView, &LogView::log);
    m_eegThread.start();

    /******************** Wristband ********************/
    connect(ui_settingView, &SettingView::requestStartBandService, this, [=](const int port) {
        ui_bandView->setUIConnecting(tr("Starting service..."));
        m_bandServer->start(port);
    });
    connect(ui_settingView, &SettingView::requestStopBandService, this, [=] {
        ui_bandView->setUIDisconnecting(tr("Stopping service..."));
        m_bandServer->stop();
    });
    connect(m_bandServer, &WristbandServer::runningChanged, this, [=](const bool isRunning) {
        if (isRunning) {
            ui_settingView->onBandServiceStarted();
            ui_bandView->setUIConnected(tr("Service Running."));
            ui_bandView->log(tr("Wristband service started successfully."), LogEdit::Success);
        } else {
            ui_settingView->onBandServiceStopped();
            ui_bandView->setUIDisconnected();
            ui_bandView->log(tr("Wristband service stopped."), LogEdit::Info);
        }
    });
    connect(m_bandServer, &WristbandServer::logFetched, ui_bandView, &LogView::log);


    /******************** Camera ********************/
    //打开摄像头
    connect(ui_settingView, &SettingView::requestOpenCamera, ui_videowidget, &VideoWidget::startVideo);
    //关闭摄像头
    connect(ui_settingView, &SettingView::requestCloseCamera, ui_videowidget, &VideoWidget::stopVideo);
    // connect(ui_settingView, &SettingView::requestOpenCamera, ui_videowidget, &VideoWidget::initrtmp);
    // connect(ui_settingView, &SettingView::cameraDeviceChanged, this, [=] {
    //     ui_cameraView->updateCamera(ui_settingView->cameraDevice(), ui_settingView->cameraFormat());
    // });
    // connect(ui_settingView, &SettingView::cameraFormatChanged, ui_cameraView, &CameraView::updateFormat);
}
