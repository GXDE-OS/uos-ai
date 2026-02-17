#ifndef BASECLIPBOARD_H
#define BASECLIPBOARD_H
#include "uosai_global.h"

#include <QObject>

namespace uos_ai {

class BaseClipboard : public QObject
{
    Q_OBJECT
public:
    static BaseClipboard *instance();
    static BaseClipboard *ttsInstance();
    virtual QString getClipText() = 0;
    virtual void clearClipText() = 0;
    virtual void setClipText(const QString &text) = 0;
    virtual bool isScribeWordsVisible() = 0;
    virtual void blockChangedSignal(bool) = 0;

Q_SIGNALS:
    void selectWords();
protected:
    BaseClipboard(QObject *parent = nullptr);
protected:
    QString clipText = "";
};

}
#endif // BASECLIPBOARD_H
