#ifndef TREELANDCLIPBOARD_H
#define TREELANDCLIPBOARD_H

#include "datacontrolmanagerv1.h"
#include "uosai_global.h"
#include "private/baseclipboard.h"

#include <QClipboard>
#include <QObject>

class DataControlDeviceV1ManagerV1;
class DataControlDeviceV1;
namespace uos_ai {
class TreelandClipboard : public BaseClipboard
{
    Q_OBJECT
public:
    TreelandClipboard(QObject *parent = nullptr);
    QString getClipText() override;
    void clearClipText() override;
    void setClipText(const QString &text) override;
    bool isScribeWordsVisible() override;
    void blockChangedSignal(bool) override;

private:
    std::unique_ptr<DataControlDeviceV1ManagerV1> m_manager;
    std::unique_ptr<DataControlDeviceV1> m_device;

    bool isValid();

Q_SIGNALS:
    void changed(QClipboard::Mode mode);
};
}

#endif //TREELANDCLIPBOARD_H
