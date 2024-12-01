#ifndef AIASSISTANTTRANSPROXY_H
#define AIASSISTANTTRANSPROXY_H

#include "uosai_global.h"

#include <QObject>

UOSAI_BEGIN_NAMESPACE
class AiassistantSubstitute;
class AiassistantTransProxy : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.iflytek.aiassistant.trans")
public:
    inline QString proxyPath() const {
        return "/aiassistant/trans";
    }
public:
    explicit AiassistantTransProxy(AiassistantSubstitute *parent = nullptr);
    ~AiassistantTransProxy();
public slots:

public:
    void setTransEnable(bool on);
    bool getTransEnable();
    void setTransLanguage(const QString &language);
    QString getTransLanguage();
private:
    AiassistantSubstitute *q;
};

UOSAI_END_NAMESPACE

#endif // AIASSISTANTTRANSPROXY_H
