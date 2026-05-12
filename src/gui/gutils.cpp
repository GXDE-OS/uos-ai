#include "gutils.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

bool GUtils::connection(const QObject *sender, const char *signal, const QObject *receiver, const char *method, Qt::ConnectionType type)
{
    const int signalIndex = sender->metaObject()->indexOfSignal(signal);
    const int slotIndex = receiver->metaObject()->indexOfSlot(method);
    if (signalIndex == -1 || slotIndex == -1) {
        qCWarning(logAIGUI) << "Signal or slot not found - signal:" << signal << "slot:" << method;
        return false;
    }

    const QMetaMethod signalMethod = sender->metaObject()->method(signalIndex);
    const QMetaMethod slotMethod = receiver->metaObject()->method(slotIndex);
    bool result = QObject::connect(sender, signalMethod, receiver, slotMethod, type);
    
    if (result) {
        qCDebug(logAIGUI) << "Signal connection established successfully - signal:" << signalMethod.name() << "slot:" << slotMethod.name();
    } else {
        qCWarning(logAIGUI) << "Failed to establish signal connection - signal:" << signalMethod.name() << "slot:" << slotMethod.name();
    }
    
    return result;
}

// 是否是紧凑模式
bool GUtils::isCompactMode()
{
    int index = DGuiApplicationHelper::instance()->metaObject()->indexOfMethod("isCompactMode()");
    if (index == -1) {
        qCWarning(logAIGUI) << "isCompactMode method not found in DGuiApplicationHelper";
        return false;
    }

    QMetaMethod method = DGuiApplicationHelper::instance()->metaObject()->method(index);
    bool returnValue;
    bool invokeResult = method.invoke(DGuiApplicationHelper::instance(), Qt::DirectConnection, Q_RETURN_ARG(bool, returnValue));
    
    if (!invokeResult) {
        qCWarning(logAIGUI) << "Failed to invoke isCompactMode method";
        return false;
    }
    
    qCDebug(logAIGUI) << "Compact mode status:" << returnValue;
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

    QString result = "data:image/png;base64," + byteArray.toBase64();
    qCDebug(logAIGUI) << "Image generated successfully, base64 length:" << byteArray.toBase64().length();
    return result;
}
