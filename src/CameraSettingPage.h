//
// Created by liu_zongchang on 2025/1/13 18:09.
// Email 1439797751@qq.com
// 
//

#ifndef CAMERA_SETTING_PAGE_H
#define CAMERA_SETTING_PAGE_H

#include "ElaText.h"
#include "ElaScrollArea.h"
#include "ElaPushButton.h"
#include "UVC_Win_DShow.h"


class QLabel;

class CameraSettingPage final : public QWidget {
    Q_OBJECT
public:
    explicit CameraSettingPage(QWidget *parent = nullptr);
    ~CameraSettingPage() override = default;

    void clearParameter() const;

    void updateParameter(const CameraDevice& cameraInfo,int width, int height, int fps);

    void addParameterItem(const DeviceParameter& parameter);

    Q_SIGNAL void sigParameterChanged(ParameterType property, int value, bool isAuto);
private:
    ElaText *m_cameraInfoLabel;
    ElaPushButton *m_resetButton;
    QWidget *m_scrollWidget;
    ElaScrollArea *m_scrollPage;
    CameraDevice m_cameraInfo;
    std::vector<DeviceParameter> m_parameters;
};



#endif //CAMERA_SETTING_PAGE_H
