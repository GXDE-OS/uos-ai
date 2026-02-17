#ifndef XCLIPBOARD_H
#define XCLIPBOARD_H
#include "uosai_global.h"

#include "baseclipboard.h"

#include <QObject>
#include <QClipboard>

namespace uos_ai {

class XClipboard : public BaseClipboard
{
    Q_OBJECT
public:
    XClipboard(QObject *parent = nullptr);
    QString getClipText() override;
    void clearClipText() override;
    void setClipText(const QString &text) override;
    bool isScribeWordsVisible() override;
    void blockChangedSignal(bool) override;

private:

    QClipboard *selectClip = nullptr;
};
}
#endif // XCLIPBOARD_H
