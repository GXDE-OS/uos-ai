#ifndef DDEDOCKOBJECT_H
#define DDEDOCKOBJECT_H

#include <QSharedPointer>
#include <QDBusArgument>

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

private:
    QSharedPointer<QDBusInterface> m_dbus;
};

#endif // DDEDOCKOBJECT_H
