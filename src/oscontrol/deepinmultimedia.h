#ifndef DEEPINMULTIMEDIA_H
#define DEEPINMULTIMEDIA_H

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>

namespace uos_ai {
class DeepinMultimedia : public QObject
{
    Q_OBJECT
public:
    explicit DeepinMultimedia(QObject *parent = nullptr);
    virtual ~DeepinMultimedia();

    // 音乐播放器功能
    bool stateControl(const QString &control, QString &errorInfo);
    bool seek(int offset, QString &errorInfo);

private:
    // DBus 调用辅助函数
    bool callMusicDBusMethod(
            QDBusInterface *interface,
            const QString &method,
            const QVariantList &args,
            QString &errorInfo);
private:
    // 音乐播放器 DBus 接口
    QScopedPointer<QDBusInterface> m_musicProxy;
};
}
#endif // DEEPINMULTIMEDIA_H 
