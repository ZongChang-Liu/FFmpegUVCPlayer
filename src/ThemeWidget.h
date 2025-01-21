//
// Created by liu_zongchang on 2025/1/13 23:17.
// Email 1439797751@qq.com
// 
//

#ifndef THEME_WIDGET_H
#define THEME_WIDGET_H

#include <ElaTheme.h>
#include <QWidget>

class ThemeWidget : public QWidget {
    Q_OBJECT
public:
    explicit ThemeWidget(QWidget *parent = nullptr);
    ~ThemeWidget() override = default;

protected:
    void paintEvent(QPaintEvent *event) override;
private:
    ElaTheme *m_theme;
    bool m_isEnableMica;
};



#endif //THEME_WIDGET_H
