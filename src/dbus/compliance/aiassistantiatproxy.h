#ifndef AIASSISTANTIATPROXY_H
#define AIASSISTANTIATPROXY_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AiassistantSubstitute;
class AiassistantIatProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.iflytek.aiassistant.iat")
public:
    explicit AiassistantIatProxy(AiassistantSubstitute *parent = nullptr);
    ~AiassistantIatProxy();
    inline QString proxyPath() const {
        return "/aiassistant/iat";
    }
public :
    void setIatEnable(bool on);

    bool getIatEnable();
// unknown
//    bool setEos(int eos);
//    int getEos();
//    bool setBos(int bos);
//    int getBos();
//    bool setBosWarning(int warningTime);
//    int getBosWarning();
private:
    AiassistantSubstitute *q;
};

UOSAI_END_NAMESPACE

#endif // AIASSISTANTIATPROXY_H
