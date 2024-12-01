#ifndef OSCALLCONTEXT_H
#define OSCALLCONTEXT_H

#include <QString>

struct OSCallContext {
    enum CallError {
        NonError = 0,
        NotImpl  = 1,
        NonService,
        InvalidArgs,
        AppNotFound,
        AppStartFailed
    } error;

    QString errorInfo;
    QString output;
};


#endif // OSCALLCONTEXT_H
