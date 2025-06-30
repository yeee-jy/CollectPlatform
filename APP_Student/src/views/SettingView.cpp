#include "SettingView.h"
#include <QGroupBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QListView>
#include <QPushButton>
#include <QMessageBox>

#include "components/IPv4Edit.h"
#include "services/SettingsManager.h"

static QString format2String(const QCameraFormat& cameraFormat) {
    return QString("%1×%2@%3fps, %4")
            .arg(cameraFormat.resolution().width())
            .arg(cameraFormat.resolution().height())
            .arg(cameraFormat.maxFrameRate())
            .arg(QVideoFrameFormat::pixelFormatToString(cameraFormat.pixelFormat()));
}

CameraSettingManager::CameraSettingManager(SettingView* parent)
    : QObject(parent),
      ui_deviceComboBox(parent->ui_cameraDeviceComboBox),
      ui_formatComboBox(parent->ui_cameraFormatComboBox),
      ui_openBtn(parent->ui_cameraOpenBtn) {
    m_cameraDevice = QMediaDevices::defaultVideoInput();
    connect(&m_mediaDevices, &QMediaDevices::videoInputsChanged, this, &CameraSettingManager::updateDeviceComboBox);
    connect(this, &CameraSettingManager::deviceChanged, this, &CameraSettingManager::updateFormatComboBox);
    connect(this, &CameraSettingManager::deviceChanged, this, &CameraSettingManager::updateOpenBtn);
    connect(ui_deviceComboBox, &QComboBox::currentTextChanged, this, &CameraSettingManager::updateCurrentDevice);
    connect(ui_formatComboBox, &QComboBox::currentTextChanged, this, &CameraSettingManager::updateCurrentFormat);
    updateDeviceComboBox();
    updateFormatComboBox();
    updateOpenBtn();
}

QCameraDevice CameraSettingManager::device() const {
    return m_cameraDevice;
}

QCameraFormat CameraSettingManager::format() const {
    return m_cameraFormat;
}

void CameraSettingManager::updateCurrentDevice() {
    setDevice(qvariant_cast<QCameraDevice>(ui_deviceComboBox->currentData()));
}

void CameraSettingManager::updateCurrentFormat() {
    setFormat(qvariant_cast<QCameraFormat>(ui_formatComboBox->currentData()));
}

void CameraSettingManager::setDevice(const QCameraDevice& device) {
    if (m_cameraDevice == device)
        return;
    m_cameraDevice = device;
    emit deviceChanged(device);
}

void CameraSettingManager::setFormat(const QCameraFormat& format) {
    if (m_cameraFormat == format)
        return;
    m_cameraFormat = format;
    emit formatChanged(format);
}

void CameraSettingManager::updateDeviceComboBox() {
    disconnect(ui_deviceComboBox, &QComboBox::currentTextChanged, this, &CameraSettingManager::updateCurrentDevice);

    ui_deviceComboBox->clear();
    auto allDevices = QMediaDevices::videoInputs();
    const QCameraDevice defaultDevice = QMediaDevices::defaultVideoInput();
    bool isOldStillExist = false;
    for (const auto& device: allDevices) {
        ui_deviceComboBox->addItem(device.description(), QVariant::fromValue(device));
        isOldStillExist = isOldStillExist || (device == m_cameraDevice);
    }

    if (isOldStillExist) {
        ui_deviceComboBox->setCurrentText(m_cameraDevice.description());
    } else if (!defaultDevice.isNull()) {
        ui_deviceComboBox->setCurrentText(defaultDevice.description());
    }
    updateCurrentDevice();

    connect(ui_deviceComboBox, &QComboBox::currentTextChanged, this, &CameraSettingManager::updateCurrentDevice);
}

void CameraSettingManager::updateFormatComboBox() {
    disconnect(ui_formatComboBox, &QComboBox::currentTextChanged, this, &CameraSettingManager::updateCurrentFormat);

    ui_formatComboBox->clear();
    if (!m_cameraDevice.isNull()) {
        auto allFormats = m_cameraDevice.videoFormats();
        for (const auto& format: allFormats) {
            ui_formatComboBox->addItem(format2String(format), QVariant::fromValue(format));
        }
    }
    updateCurrentFormat();

    connect(ui_formatComboBox, &QComboBox::currentTextChanged, this, &CameraSettingManager::updateCurrentFormat);
}

void CameraSettingManager::updateOpenBtn() {
    if (m_cameraDevice.isNull()) {
        ui_openBtn->setEnabled(false);
    } else {
        ui_openBtn->setEnabled(true);
    }
}

SettingView::SettingView(QWidget* parent)
    : QWidget(parent) {
    initUI();
    initConnection();
}

QCameraDevice SettingView::cameraDevice() const {
    return m_cameraManager->device();
}

QCameraFormat SettingView::cameraFormat() const {
    return m_cameraManager->format();
}

void SettingView::onEEGConnected() const {
    ui_eegConnectBtn->setText(tr("Disconnect"));
    ui_eegConnectBtn->setEnabled(true);
}

void SettingView::onEEGDisconnected() const {
    ui_eegAddressEdit->setEnabled(true);
    ui_eegPortSpinBox->setEnabled(true);
    ui_eegConnectBtn->setText(tr("Connect"));
    ui_eegConnectBtn->setEnabled(true);
}

void SettingView::onBandServiceStarted() const {
    ui_bandListenBtn->setText(tr("Stop Service"));
    ui_bandListenBtn->setEnabled(true);
}

void SettingView::onBandServiceStopped() const {
    ui_bandPortSpinBox->setEnabled(true);
    ui_bandListenBtn->setText(tr("Start Service"));
    ui_bandListenBtn->setEnabled(true);
}

void SettingView::onEEGConnectBtnClicked() {
    if (ui_eegConnectBtn->text() == tr("Connect")) {
        if (ui_eegAddressEdit->address().isEmpty()) {
            QMessageBox::warning(this, tr("Warning"), tr("Please enter a valid EEG address."));
            return;
        }
        ui_eegConnectBtn->setText(tr("Connecting..."));
        ui_eegConnectBtn->setEnabled(false);
        ui_eegAddressEdit->setEnabled(false);
        ui_eegPortSpinBox->setEnabled(false);
        emit requestConnectEEG(ui_eegAddressEdit->address(), ui_eegPortSpinBox->value());
    } else if (ui_eegConnectBtn->text() == tr("Disconnect")) {
        ui_eegConnectBtn->setText(tr("Disconnecting..."));
        ui_eegConnectBtn->setEnabled(false);
        emit requestDisconnectEEG();
    }
}

void SettingView::onBandListenBtnClicked() {
    if (ui_bandListenBtn->text() == tr("Start Service")) {
        ui_bandListenBtn->setText(tr("Starting..."));
        ui_bandListenBtn->setEnabled(false);
        ui_bandPortSpinBox->setEnabled(false);
        emit requestStartBandService(ui_bandPortSpinBox->value());
    } else if (ui_bandListenBtn->text() == tr("Stop Service")) {
        ui_bandListenBtn->setText(tr("Stopping..."));
        ui_bandListenBtn->setEnabled(false);
        emit requestStopBandService();
    }
}

void SettingView::onCameraOpenBtnClicked() {
    if (ui_cameraOpenBtn->text() == tr("Open")) {
        ui_cameraOpenBtn->setText(tr("Close"));
        emit requestOpenCamera();
    } else if (ui_cameraOpenBtn->text() == tr("Close")) {
        ui_cameraOpenBtn->setText(tr("Open"));
        emit requestCloseCamera();
    }
}

void SettingView::initUI() {
    ui_eegAddressEdit = new IPv4Edit();
    ui_eegAddressEdit->setAddress(SettingsManager::instance().eegHost());
    ui_eegAddressEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui_eegPortSpinBox = new QSpinBox();
    ui_eegPortSpinBox->setAlignment(Qt::AlignCenter);
    ui_eegPortSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui_eegPortSpinBox->setRange(1024, 65535);
    ui_eegPortSpinBox->setValue(SettingsManager::instance().eegPort());
    ui_eegConnectBtn = new QPushButton(tr("Connect"));

    ui_bandPortSpinBox = new QSpinBox();
    ui_bandPortSpinBox->setAlignment(Qt::AlignCenter);
    ui_bandPortSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui_bandPortSpinBox->setRange(1024, 65535);
    ui_bandPortSpinBox->setValue(SettingsManager::instance().bandPort());
    ui_bandListenBtn = new QPushButton(tr("Start Service"));

    ui_cameraDeviceComboBox = new QComboBox();
    ui_cameraDeviceComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui_cameraFormatComboBox = new QComboBox();
    ui_cameraFormatComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui_cameraOpenBtn = new QPushButton(tr("Open"));

    auto* eegGroup = new QGroupBox(tr("EEG"));
    auto* eegLayout = new QFormLayout(eegGroup);
    eegLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    eegLayout->addRow(tr("Host"), ui_eegAddressEdit);
    eegLayout->addRow(tr("Port"), ui_eegPortSpinBox);
    eegLayout->addRow(ui_eegConnectBtn);

    auto* bandGroup = new QGroupBox(tr("Wristband"));
    auto* bandLayout = new QFormLayout(bandGroup);
    bandLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    bandLayout->addRow(tr("Port"), ui_bandPortSpinBox);
    bandLayout->addRow(ui_bandListenBtn);

    auto* cameraGroup = new QGroupBox(tr("Camera"));
    auto* cameraLayout = new QFormLayout(cameraGroup);
    cameraLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    cameraLayout->addRow(tr("Device"), ui_cameraDeviceComboBox);
    cameraLayout->addRow(tr("Format"), ui_cameraFormatComboBox);
    cameraLayout->addRow(ui_cameraOpenBtn);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(eegGroup);
    layout->addWidget(bandGroup);
    layout->addWidget(cameraGroup);
    layout->addStretch(1);
}

void SettingView::initConnection() {
    m_cameraManager = new CameraSettingManager(this);

    connect(ui_eegConnectBtn, &QPushButton::clicked, this, &SettingView::onEEGConnectBtnClicked);
    connect(ui_bandListenBtn, &QPushButton::clicked, this, &SettingView::onBandListenBtnClicked);
    //打开摄像头
    connect(ui_cameraOpenBtn, &QPushButton::clicked, this, &SettingView::onCameraOpenBtnClicked);

    connect(m_cameraManager, &CameraSettingManager::deviceChanged, this, &SettingView::cameraDeviceChanged);
    connect(m_cameraManager, &CameraSettingManager::formatChanged, this, &SettingView::cameraFormatChanged);
}
