//
// Created by liu_zongchang on 2025/1/7 23:04.
// Email 1439797751@qq.com
// 
//

#include "MyMainWindows.h"
#include "ElaDockWidget.h"
#include "ElaStatusBar.h"
#include "ElaMenuBar.h"
#include <QKeyEvent>
#include <QVBoxLayout>

#include "CameraSettingPage.h"
#include "Microscope_Utils_Log.h"
#include "CameraSwitchingPage.h"
#include "FFmpegPlayer.h"
#include "VideoWidget.h"

MyMainWindows::MyMainWindows(QWidget* parent) : ElaWindow(parent)
{
    this->setWindowIcon(QIcon(":/resources/logo.png"));
    this->setMinimumSize(600, 400);
    this->setWindowTitle(tr("微笑 - 不翻身的咸鱼"));
    this->setIsStayTop(false);
    this->setIsNavigationBarEnable(false);
    this->setIsCentralStackedWidgetTransparent(true);
    this->setWindowButtonFlags(
        ElaAppBarType::MinimizeButtonHint | ElaAppBarType::MaximizeButtonHint | ElaAppBarType::CloseButtonHint |
        ElaAppBarType::ThemeChangeButtonHint);
    this->moveToCenter();

    initSystem();
    initEdgeLayout();
    initContentLayout();


    m_ffmpegPlayer = new FFmpegPlayer(this);
    connect(m_ffmpegPlayer, &FFmpegPlayer::sigFrameReaded, m_videoWidget, &VideoWidget::onUpdateFrame);


    connect(m_cameraSwitchingPage, &CameraSwitchingPage::sigCameraChanged, this, &MyMainWindows::onCameraChanged);
    connect(m_cameraSwitchingPage, &CameraSwitchingPage::sigRecord, m_videoWidget, &VideoWidget::setRecordFlag);
    connect(m_cameraSwitchingPage, &CameraSwitchingPage::sigScreenShot, [this]() { LOG_DEBUG("ScreenShot"); });
    m_cameraSwitchingPage->updateCameraInfo();
}

MyMainWindows::~MyMainWindows()
{
    if (m_ffmpegPlayer)
    {
        m_ffmpegPlayer->close();
        delete m_ffmpegPlayer;
    }
}

void MyMainWindows::initSystem() const
{
#ifdef Q_OS_WIN
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = CAMERA;
    //注册相机设备
    HDEVNOTIFY hDevNotify = RegisterDeviceNotification(reinterpret_cast<HANDLE>(this->winId()), &NotificationFilter,
                                                       DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hDevNotify == nullptr)
    {
        LOG_ERROR("CAMERA Register Failed");
    }
    else
    {
        LOG_INFO("CAMERA Register Success");
    }

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = COMPORT;
    //注册串口设备
    hDevNotify = RegisterDeviceNotification(reinterpret_cast<HANDLE>(this->winId()), &NotificationFilter,
                                            DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hDevNotify == nullptr)
    {
        LOG_ERROR("COM Register Failed");
    }
    else
    {
        LOG_INFO("COM Register Success");
    }

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid = USB;
    //注册串口设备
    hDevNotify = RegisterDeviceNotification(reinterpret_cast<HANDLE>(this->winId()), &NotificationFilter,
                                            DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hDevNotify == nullptr)
    {
        LOG_ERROR("USB Register Failed");
    }
    else
    {
        LOG_INFO("USB Register Success");
    }
#endif
}

void MyMainWindows::initEdgeLayout()
{
    auto* menuBar = new ElaMenuBar(this);
    m_docketMenu = new ElaMenu(tr("窗口"), this);
    menuBar->addMenu(m_docketMenu);
    menuBar->setFixedHeight(30);
    auto* customWidget = new QWidget(this);
    auto* customLayout = new QVBoxLayout(customWidget);
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->addWidget(menuBar);
    customLayout->addStretch();
    this->setCustomWidget(ElaAppBarType::LeftArea, customWidget);


    auto* statusBar = new ElaStatusBar(this);
    m_statusText = new ElaText(tr("系统初始化成功! "), this);
    m_statusText->setTextPixelSize(12);
    statusBar->addWidget(m_statusText);
    this->setStatusBar(statusBar);
}

void MyMainWindows::initContentLayout()
{
    m_contentWidget = new QWidget(this);
    m_contentWidget->setObjectName("MainContent");
    auto* layout = new QVBoxLayout(m_contentWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);
    addPageNode("Main", m_contentWidget, ElaIconType::House);

    m_videoWidget = new VideoWidget(this);
    m_contentWidget->layout()->addWidget(m_videoWidget);

    m_cameraSwitchingPage = new CameraSwitchingPage(this);
    createDockWidget(tr("相机设置"), m_cameraSwitchingPage, Qt::RightDockWidgetArea);
    m_cameraSettingPage = new CameraSettingPage(this);
    createDockWidget(tr("相机参数"), m_cameraSettingPage, Qt::RightDockWidgetArea);
}

void MyMainWindows::createDockWidget(const QString& title, QWidget* widget, const Qt::DockWidgetArea area)
{
    auto* dockWidget = new ElaDockWidget(title, this);
    dockWidget->setObjectName(title);
    widget->setParent(dockWidget);
    dockWidget->setDockWidgetTitleIconVisible(false);
    dockWidget->setWidget(widget);
    this->addDockWidget(area, dockWidget);
    this->resizeDocks({dockWidget}, {250}, Qt::Horizontal);

    auto* action = dockWidget->toggleViewAction();
    action->setText(title);
    action->setCheckable(true);
    action->setChecked(true);
    m_docketMenu->addAction(action);
}

void MyMainWindows::onCameraChanged(const CameraDevice& cameraDevice, const int width, const int height, const int fps,
                                    const QString& format) const
{
    QString ffmpegPath = cameraDevice.MonikerName;
    ffmpegPath.replace(":", "_");
    LOG_INFO("path: {} width: {} height: {} fps: {} format: {}", ffmpegPath.toStdString().c_str(), width, height, fps,
             format.toStdString().c_str());
    m_ffmpegPlayer->playDevice(ffmpegPath, width, height, fps, format.toLower());
    m_cameraSettingPage->updateParameter(cameraDevice, width, height, fps);
}

void MyMainWindows::keyPressEvent(QKeyEvent* event)
{
    QMainWindow::keyPressEvent(event);
    if (event->key() == Qt::Key_Escape)
    {
        close();
    }
}

#ifdef Q_OS_WIN
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool MyMainWindows::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#else
bool MyMainWindows::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#endif

    Q_UNUSED(eventType)
    MSG* msg = static_cast<MSG*>(message);
    switch (msg->message)
    {
    ///////////////////////////这个是设备变化的消息////////////////////////////
    case WM_DEVICECHANGE:
        {
            if (msg->wParam == DBT_DEVICEARRIVAL || msg->wParam == DBT_DEVICEREMOVECOMPLETE)
            {
                if (const auto dev_broadcast_hdr = reinterpret_cast<PDEV_BROADCAST_HDR>(msg->lParam); dev_broadcast_hdr
                    ->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
                {
                    if (const auto dev_broadcast_device_interface = reinterpret_cast<PDEV_BROADCAST_DEVICEINTERFACE>(
                        dev_broadcast_hdr); dev_broadcast_device_interface->dbcc_classguid == CAMERA)
                    {
                        m_cameraSwitchingPage->updateCameraInfo();
                    }
                }
            }
            break;
        }
    }

    return ElaWindow::nativeEvent(eventType, message, result);
}

#endif
