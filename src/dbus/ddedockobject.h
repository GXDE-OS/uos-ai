#ifndef DDEDOCKOBJECT_H
#define DDEDOCKOBJECT_H

#include <QSharedPointer>
#include <QDBusArgument>
#include <QRect>

struct DockRect {
    int x;
    int y;
    int w;
    int h;

    QRect rect() const
    {
        return QRect(x, y, w, h);
    };
};

Q_DECLARE_METATYPE(DockRect)
class QDBusInterface;
class DDeDockObject : public QObject
{
    Q_OBJECT

public:
    explicit DDeDockObject(QObject *parent = nullptr);

    int position();

    int displayMode();

    QRect frontendWindowRect();

signals:
    void FrontendWindowRectChanged(QRect value) const;

private slots:
    void propertiesChanged(QString, QVariantMap, QStringList);
    void onFrontendWindowRectChanged(DockRect rect);

private:
    QSharedPointer<QDBusInterface> m_dbus;
};

#endif // DDEDOCKOBJECT_H
