//
// Created by liu_zongchang on 2025/1/13 23:17.
// Email 1439797751@qq.com
// 
//

#include "ThemeWidget.h"
#include <QPainter>
#include "ElaApplication.h"
#include "ElaTheme.h"

ThemeWidget::ThemeWidget(QWidget* parent) : QWidget(parent)
{
    m_theme = eTheme;
    m_isEnableMica = eApp->getIsEnableMica();
    connect(eApp, &ElaApplication::pIsEnableMicaChanged, this, [=]() {
        m_isEnableMica = eApp->getIsEnableMica();
        update();
    });
    eApp->syncMica(this);
}

void ThemeWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (!m_isEnableMica)
    {
        QPainter painter(this);
        painter.save();
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ElaThemeColor(eTheme->getThemeMode(), WindowBase));
        painter.drawRect(rect());
        painter.restore();
    }
}
