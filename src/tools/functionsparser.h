#ifndef FUNCTIONSPARSER_H
#define FUNCTIONSPARSER_H

#include <QThread>
#include <QJsonObject>

class FunctionsParser : public QThread
{
    Q_OBJECT

public:
    FunctionsParser(const QJsonObject &function);
    ~FunctionsParser();

    /**
     * @brief exitCode
     * @return
     */
    int exitCode();

    /**
     * @brief outputString
     * @return
     */
    QString outputString();

    QString directOutput() const;

private:
    /**
     * @brief sendMailFunction
     * @param arguments
     * @return
     */
    void sendMailFunction(const QJsonObject &argumentsObj);

    /**
     * @brief openUrl
     * @param arguments
     */
    void openUrl(const QString &url);

    /**
     * @brief openExec
     * @param mimeType
     */
    void openExec(const QString &mimeType);

protected:
    /**
     * @brief run
     */
    void run();

private:
    int m_exitCode = 0;
    QString m_outputString;
    QString m_directOutput;
    QJsonObject m_function;
};

#endif // FUNCTIONSPARSER_H
