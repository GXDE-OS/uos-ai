#ifndef GUTILS_H
#define GUTILS_H

#include <QObject>
#include <QMetaMethod>
#include <QWidget>
#include <QPainter>
#include <QStyleOption>
#include <QApplication>
#include <QBuffer>

#include <DGuiApplicationHelper>
#include <DApplicationHelper>

DGUI_USE_NAMESPACE

class GUtils
{
public:

    /**
     * @brief qt自带connect如果信号或者槽不存在会存在编译不通过的问题，此connect规避这个问题，但是可读性和维护性较差，慎用
     * @param sender
     * @param signal 带参数类型的信号字符串
     * @param receiver
     * @param method 带参数类型的槽字符串
     * @param type
     * @return
     */
    static bool connection(const QObject *sender, const char *signal, const QObject *receiver, const char *method, Qt::ConnectionType type = Qt::AutoConnection);

    /**
     * @brief 是不是紧凑模式
     * @return
     */
    static bool isCompactMode();

    static QString generateImage(const QWidget *widget, const QColor &color, const QSize &size, QStyle::PrimitiveElement pe);
};

#endif // GUTILS_H
