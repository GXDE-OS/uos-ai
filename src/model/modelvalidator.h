#ifndef MODELVALIDATOR_H
#define MODELVALIDATOR_H

#include <QObject>
#include <QString>
#include <QNetworkReply>

#include "modelinfo.h"
#include "global_define.h"

namespace uos_ai {

class ModelValidator : public QObject
{
    Q_OBJECT
public:
    struct ModelValidationResult {
        bool success;
        QString errorMessage;
        GErrorType errorType;

        ModelValidationResult()
            : success(false)
            , errorType(NoError)
        {}
    };

    explicit ModelValidator(QObject *parent = nullptr);
    ~ModelValidator() override;

    ModelValidationResult validate(const ModelAccountPtr &account);
private:
    ModelValidationResult validateChatModel(const ModelAccountPtr &account);
    QString parseErrorResponse(const QByteArray &data);
};

}

#endif // MODELVALIDATOR_H
