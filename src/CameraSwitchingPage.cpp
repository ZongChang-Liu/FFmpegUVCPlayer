//
// Created by liu_zongchang on 2025/1/13 0:40.
// Email 1439797751@qq.com
// 
//
#include <QVBoxLayout>
#include "CameraSwitchingPage.h"
#include "ElaText.h"
#include "Microscope_Utils_Log.h"

CameraSwitchingPage::CameraSwitchingPage(QWidget* parent) : QWidget(parent)
{
    this->setObjectName("CameraSettingPage");
    auto* layout = new QVBoxLayout(this);
    this->setLayout(layout);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
    layout->setSpacing(10);

    QFont font;
    font.setPixelSize(12);
    font.setFamily("Microsoft YaHei");

    const auto cameraLabel = new ElaText(tr("相机"), this);
    cameraLabel->setFont(font);
    m_cameraComboBox = new ElaComboBox(this);
    m_cameraComboBox->setFont(font);
    m_cameraComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* cameraLayout = new QHBoxLayout;
    cameraLayout->setSpacing(10);
    cameraLayout->addWidget(cameraLabel);
    cameraLayout->addWidget(m_cameraComboBox);

    const auto formatLabel = new ElaText(tr("格式"), this);
    formatLabel->setFont(font);
    m_formatComboBox = new ElaComboBox(this);
    m_formatComboBox->setFont(font);
    m_formatComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    const auto formatLayout = new QHBoxLayout;
    formatLayout->setSpacing(10);
    formatLayout->addWidget(formatLabel);
    formatLayout->addWidget(m_formatComboBox);

    const auto resolutionLabel = new ElaText(tr("分辨率"), this);
    resolutionLabel->setFont(font);
    m_resolutionComboBox = new ElaComboBox(this);
    m_resolutionComboBox->setFont(font);
    m_resolutionComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    const auto resolutionLayout = new QHBoxLayout;
    resolutionLayout->setSpacing(10);
    resolutionLayout->addWidget(resolutionLabel);
    resolutionLayout->addWidget(m_resolutionComboBox);

    m_screenShotButton = new ElaPushButton(tr("截图"), this);
    m_screenShotButton->setFont(font);
    m_screenShotButton->setFixedWidth(80);
    m_recordButton = new ElaToggleButton(tr("录制"), this);
    m_recordButton->setFont(font);
    m_recordButton->setFixedWidth(80);
    const auto buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(10);
    buttonLayout->addWidget(m_screenShotButton);
    buttonLayout->addWidget(m_recordButton);

    layout->addItem(cameraLayout);
    layout->addItem(formatLayout);
    layout->addItem(resolutionLayout);
    layout->addItem(buttonLayout);
    layout->addStretch();


    connect(m_cameraComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CameraSwitchingPage::onCameraChanged);
    connect(m_formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CameraSwitchingPage::onFormatChanged);
    connect(m_resolutionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CameraSwitchingPage::onResolutionChanged);
    connect(m_screenShotButton, &ElaPushButton::clicked, this, &CameraSwitchingPage::sigScreenShot);
    connect(m_recordButton, &ElaToggleButton::toggled, this, &CameraSwitchingPage::sigRecord);
}

CameraSwitchingPage::~CameraSwitchingPage()
{
    m_cameraList.clear();
}

void CameraSwitchingPage::updateCameraInfo()
{
    UVC_Win_DShow::getDevices(m_cameraList);
    m_cameraComboBox->clear();
    for (const auto& camera : m_cameraList) {
        m_cameraComboBox->addItem(camera.FriendlyName);
    }
}

void CameraSwitchingPage::setRecordButtonEnable(const bool flag) const
{
    m_recordButton->setEnabled(flag);
}

void CameraSwitchingPage::setScreenShotButtonEnable(const bool flag) const
{
    m_screenShotButton->setEnabled(flag);
}

void CameraSwitchingPage::setRecordButtonChecked(const bool flag) const
{
    if (m_recordButton->getIsToggled() == flag) {
        return;
    }
    m_recordButton->setIsToggled(flag);
}

void CameraSwitchingPage::onCameraChanged(const int index)
{
    if (index < 0 || index >= m_cameraList.size()) {
        return;
    }

    UVC_Win_DShow::getResolutions(m_cameraList[index], m_resolutionList);

    m_formatComboBox->clear();
    for (const auto& resolution : m_resolutionList) {
        if (QString format = MediaSubTypes.at(resolution.mediaSubType); m_formatComboBox->findText(format) == -1) {
            m_formatComboBox->addItem(format);
        }
    }
}

void CameraSwitchingPage::onFormatChanged(const int index) const
{
    if (index < 0) {
        return;
    }

    m_resolutionComboBox->clear();
    for (const auto& [mediaSubType, width, height, fps] : m_resolutionList) {
        if (MediaSubTypes.at(mediaSubType) == m_formatComboBox->currentText()) {
            m_resolutionComboBox->addItem(QString("%1x%2 %3fps").arg(width).arg(height).arg(fps));
        }
    }
}

void CameraSwitchingPage::onResolutionChanged(const int index)
{
    if (index < 0) {
        return;
    }


    const auto format = m_formatComboBox->currentText();
    const auto resolution = m_resolutionComboBox->currentText();
    int width = resolution.split("x")[0].toInt();
    int height = resolution.split("x")[1].split(" ")[0].toInt();
    double fps = resolution.split(" ")[1].split("fps")[0].toDouble();

    Q_EMIT sigCameraChanged(m_cameraList[m_cameraComboBox->currentIndex()],width, height, fps, format);
    LOG_DEBUG("sigCameraChanged camera path:{} width:{} height:{} fps:{} format:{}", m_cameraList[m_cameraComboBox->currentIndex()].MonikerName, width, height, fps, format.toStdString());
}