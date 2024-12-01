#ifndef ATSPIDESKTOP_H
#define ATSPIDESKTOP_H

#include "uosai_global.h"

#include <QObject>
#include <QThread>
#include <QString>
#include <QDebug>
#include <QMutex>
#include <QQueue>
#include <QVector>
#include <QFuture>

#include <atspi/atspi.h>

UOSAI_BEGIN_NAMESPACE

enum IatAnsType {
    eIatResultType_NoPgs,
    eIatResultType_PgsRpl,
    eIatResultType_PgsApd,
};
class AtspiDesktop : public QThread
{
    Q_OBJECT
public:
    explicit AtspiDesktop();
    ~AtspiDesktop();
    virtual void run();

    bool atspiInsertText(const QString &textContext, const int &pgstype ,const bool &finish);
    QString getAtspiDebugInfo(){ return m_atspiDebugInfo;}
    QString getLastError() { return m_lastError;}

    static AtspiDesktop *getInstance();
    QString getSelectedContext();
    int getSelectedProgramPId();

    bool isCaptureTextSuccess();
private:
    static void processVectorAtspiEvent(AtspiDesktop * thisObj);
private:
    static void onEvent (AtspiEvent *event,void *data);
    void listenerRegister();
    void listenerUnRegister();//void atspi_event_quit ();
    void processEvent(const AtspiEvent *event);
    static bool isEditable(AtspiAccessible* eventSource);
    void doInsertText(const QString &InsertText);

    AtspiText                   * m_AtspiText = nullptr;
    AtspiEditableText           * m_AtspiEditableText = nullptr;

    gboolean                    m_toolkitIsGtk = false;
    gint                        m_insertCaretOffset = 0;
    QString                     m_app_name;
    bool                        m_needStopHandler = false;
    QString                     m_atspiDebugInfo;
    QString                     m_lastError;

    QFuture<void>           m_thread;
    QMutex                m_mutex;
    QVector<AtspiEvent *> m_vecAtspiEvent;
    bool                  m_loop = true;
    int                   m_seletedProgramPid{-1};
    int                   m_textFouceSuccessPid{-1};
};

UOSAI_END_NAMESPACE

#endif
