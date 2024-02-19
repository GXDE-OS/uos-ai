#include "gutils.h"

bool GUtils::connection(const QObject *sender, const char *signal, const QObject *receiver, const char *method, Qt::ConnectionType type)
{
    const int signalIndex = sender->metaObject()->indexOfSignal(signal);
    const int slotIndex = receiver->metaObject()->indexOfSlot(method);
    if (signalIndex == -1 || slotIndex == -1) {
        return false;
    }

    const QMetaMethod signalMethod = sender->metaObject()->method(signalIndex);
    const QMetaMethod slotMethod = receiver->metaObject()->method(slotIndex);
    return QObject::connect(sender, signalMethod, receiver, slotMethod, type);
}

// 是否是紧凑模式
bool GUtils::isCompactMode()
{
    int index = DGuiApplicationHelper::instance()->metaObject()->indexOfMethod("isCompactMode()");
    if (index == -1) {
        return false;
    }

    QMetaMethod method = DGuiApplicationHelper::instance()->metaObject()->method(index);
    bool returnValue;
    method.invoke(DGuiApplicationHelper::instance(), Qt::DirectConnection, Q_RETURN_ARG(bool, returnValue));

    return returnValue;
}

QString GUtils::generateImage(const QWidget *widget, const QColor &color, const QSize &size, QStyle::PrimitiveElement pe)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(color);
    painter.setBrush(color);

    QStyleOption opt;
    opt.initFrom(widget);
    opt.rect = QRect(QPoint(0, 0), size);
    opt.state = QStyle::State_Enabled;

    QApplication::style()->drawPrimitive(pe, &opt, &painter, nullptr);
    painter.end();

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    pixmap.toImage().save(&buffer, "PNG");
    return "data:image/png;base64," + byteArray.toBase64();
}
