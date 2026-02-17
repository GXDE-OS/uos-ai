#ifndef ASRWRAPPER_H
#define ASRWRAPPER_H
#include "uosai_global.h"
#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AsrModel;
class AsrWrapper :  public QObject
{
    Q_OBJECT
public:
    explicit AsrWrapper(QObject *parent = nullptr);
    ~AsrWrapper();
public slots:
    QString startAsr(const QVariantMap &param);
    void stopAsr();
signals:
    void onNotify(const QString &msg);
private:
    AsrModel *m_asrModel = nullptr;
    QString m_currentID;
};
UOSAI_END_NAMESPACE
#endif // ASRWRAPPER_H
