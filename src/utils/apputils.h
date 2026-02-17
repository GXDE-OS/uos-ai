#ifndef APPUTILS_H
#define APPUTILS_H

#include <uosai_global.h>

#include <QString>
#include <QMap>
#include <QObject>
#include <QDebug>

namespace uos_ai {

class AppUtils : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AppUtils)
public:
    static AppUtils &instance()
    {
        static AppUtils ins;
        return ins;
    }

    /**
     * @brief app是否包含在表中
     * @param app
     * @return
     */
    bool checkAppInMap(QString app)
    {
        return m_app2NameMap.contains(app);
    }

    /**
     * @brief 获取app的icon
     * @param app
     * @return
     */
    QString getIconByApp(QString app)
    {
        if (kApp2IconMap.contains(app)) {
            //qDebug() << "app:" << app << "icon:" << kApp2IconMap[app];
            return kApp2IconMap[app];
        }

        //qDebug() << "app:" << app << "not in map";
        return "";
    }

    /**
     * @brief 获取app的展示名字
     * @param app
     * @return
     */
    QString getNameByApp(QString app)
    {
        if (m_app2NameMap.contains(app)) {
            //qDebug() << "app:" << app << "name:" << kApp2NameMap[app];
            return m_app2NameMap[app];
        }

        //qDebug() << "app:" << app << "not in map";
        return "";
    }

private:
    explicit AppUtils();

    // app对应的icon
    static QMap<QString, QString> kApp2IconMap;
    // app对应的名字
    QMap<QString, QString> m_app2NameMap;
};

}
#endif // APPUTILS_H
