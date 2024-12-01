#include "atspidesktop.h"
#include "util.h"

#include <QDebug>
#include <QTextCodec>
#include <QMutexLocker>
#include <QApplication>
#include <QtConcurrent>

#include <unistd.h>
#include <glib/garray.h>

UOSAI_USE_NAMESPACE

#define ENABLE_ATSPI_DEBUG  false
#define ATSPIFREE(x) if(x!=nullptr){g_object_unref(x);x=nullptr;}

AtspiDesktop::AtspiDesktop()
{
    Q_ASSERT(currentThread() == qApp->thread());

    m_thread = QtConcurrent::run(AtspiDesktop::processVectorAtspiEvent, this);
}

AtspiDesktop::~AtspiDesktop()
{
    m_loop = false;
    listenerUnRegister();
}

bool AtspiDesktop::isEditable(AtspiAccessible* eventSource)
{
    AtspiStateSet *state_set = atspi_accessible_get_state_set (eventSource);
    GArray *states = atspi_state_set_get_states (state_set);

    for (guint i = 0; i < states->len; i++)
    {
       AtspiStateType state = static_cast<AtspiStateType>(g_array_index (states, gint, i));
       if(state == ATSPI_STATE_EDITABLE)
       {
           g_array_free(states,true);
           g_object_unref(state_set);
           return true;
       }
    }
    g_object_unref(state_set);
    g_array_free(states,true);
    return false;
}

void AtspiDesktop::doInsertText(const QString &InsertText)
{
    if(m_toolkitIsGtk)
    {
        atspi_editable_text_insert_text (m_AtspiEditableText, m_insertCaretOffset, InsertText.toUtf8().data(), InsertText.toUtf8().length(), nullptr);
        atspi_text_set_caret_offset(m_AtspiText,m_insertCaretOffset + InsertText.length(),nullptr);
    }
    else
    {
        atspi_editable_text_insert_text (m_AtspiEditableText, m_insertCaretOffset, InsertText.toUtf8().data(), InsertText.length(), nullptr);
    }
}

bool AtspiDesktop::atspiInsertText(const QString &textContext, const int &pgstype ,const bool &finish)
{
    Q_UNUSED(finish);
    if(!m_AtspiEditableText || !m_AtspiText)
    {
        return false;
    }

    AtspiRange *atspiSelectRange = atspi_text_get_selection(m_AtspiText, 0, nullptr);

    if(atspiSelectRange->start_offset != atspiSelectRange->end_offset)
    {
        qDebug() << "iat delete pos:" << atspiSelectRange->start_offset << atspiSelectRange->end_offset;
        atspi_editable_text_delete_text(m_AtspiEditableText, atspiSelectRange->start_offset, atspiSelectRange->end_offset, nullptr);
    }

    if (eIatResultType_NoPgs == pgstype && textContext.length() > 0)
    {
        m_insertCaretOffset = atspi_text_get_caret_offset (m_AtspiText,nullptr);
        doInsertText(textContext);
    }
    else if (eIatResultType_PgsApd == pgstype)
    {
          m_insertCaretOffset = atspi_text_get_caret_offset (m_AtspiText,nullptr);
          doInsertText(textContext);
    }
    else if (eIatResultType_PgsRpl == pgstype)
    {
        gint currCaretOffset = atspi_text_get_caret_offset (m_AtspiText,nullptr);
        atspi_editable_text_delete_text (m_AtspiEditableText, m_insertCaretOffset, currCaretOffset ,nullptr);
        atspi_text_set_caret_offset(m_AtspiText,m_insertCaretOffset,nullptr);
        doInsertText(textContext);
        if(ENABLE_ATSPI_DEBUG) qDebug() << "eIatResultType_PgsRpl pgs:" << textContext;
    }

    return true;
}

static void atspiEventFree (AtspiEvent *event)
{
  g_object_unref (event->source);
  g_free (event->type);
  g_value_unset (&event->any_data);
  g_free (event);
}
void AtspiDesktop::onEvent (AtspiEvent *event,void *data)
{
    if(0 != strcmp(event->type,"object:state-changed:focused") && 0 != strcmp(event->type,"window:deactivate") &&  0 != strcmp(event->type,"window:destroy")) {
        atspiEventFree(event);
        return;
    }

    AtspiDesktop* thisObj = static_cast<AtspiDesktop*>(data);
    int selectedPid = static_cast<int>(atspi_accessible_get_process_id(event->source,nullptr));

    if(selectedPid == static_cast<int>(getpid())) {
        if(ENABLE_ATSPI_DEBUG) qDebug() << "-------AT-SPI  DeepinAIAssistant return ------";
        atspiEventFree(event);
        return;
    }

    if(strcmp(event->type,"object:state-changed:focused") == 0 && !event->detail1) {
        atspiEventFree(event);
        return;
    }

    if(0 == strcmp(event->type,"object:state-changed:focused"))
    {
        thisObj->m_seletedProgramPid = selectedPid;
    }

    QMutexLocker lock(&thisObj->m_mutex);

    thisObj->m_vecAtspiEvent.push_back(event);
}

void AtspiDesktop::processEvent(const AtspiEvent *event)
{
    QString         widgetToolkitName;
    AtspiAccessible *application = nullptr;
    QString          appName;
    QString          appRoleName;
    QString          widgetRoleName;
    gchar*           widgetRoleNameTmp = nullptr;

    // 1.当消息为window:deactivate或window:destroy，但上次没有缓存句柄时，不做处理；在一定程度上规避检测进城被关闭偶现异常崩溃问题
    if((strcmp(event->type,"window:deactivate") == 0 ||  strcmp(event->type,"window:destroy") == 0) && m_app_name.isEmpty())
    {
        if(ENABLE_ATSPI_DEBUG) qDebug() << "----" << event->type << "-----" << __FUNCTION__ << __LINE__;
        return;
    }
    // 2.获取当前选中进城句柄
    application = atspi_accessible_get_application (event->source, nullptr);
    gchar* appNameTmp = atspi_accessible_get_name (application, nullptr);
    appName = appNameTmp;
    g_free(appNameTmp);
    if(ENABLE_ATSPI_DEBUG) qDebug() << "---Enter " + QString(__FUNCTION__) + "---" + \
       QString(event->type) + "---app(" + appName + ")---m_app(" + m_app_name +")---";
    // 3.失焦或者释放消息事件时，释放上次缓存的选中句柄
    if((strcmp(event->type,"window:deactivate") == 0 ||  strcmp(event->type,"window:destroy") == 0) && appName == m_app_name)
    {
        if(ENABLE_ATSPI_DEBUG) qDebug() << "---release " + appName + "---";
        ATSPIFREE(m_AtspiText);
        ATSPIFREE(m_AtspiEditableText);
        m_app_name.clear();
        m_lastError.clear();
        m_atspiDebugInfo.clear();
        m_textFouceSuccessPid = -1;
        return;
    }

    // 4.获取当前选中控件信息及框架类型信息,qt还是gtk
    gchar* roleNameTmp = atspi_accessible_get_role_name (application, nullptr);
    gchar* toolkitNameTmp = atspi_accessible_get_toolkit_name (application,nullptr);
    appRoleName = roleNameTmp;
    widgetToolkitName = toolkitNameTmp;
    g_free(roleNameTmp);
    g_free(toolkitNameTmp);

    /* We only care about focus/selection gain */
    if (!event->detail1) {
        if(ENABLE_ATSPI_DEBUG) qDebug() << " ***[event->detail1 = nullptr]";
        return;
    }

    if(ENABLE_ATSPI_DEBUG) qDebug() << "---type:" << event->type << " app:" << appName  << " appRolename:" << appRoleName << "---";

    m_lastError.clear();
    m_atspiDebugInfo.clear();
    m_app_name = appName;

    m_toolkitIsGtk = false;

    ATSPIFREE(m_AtspiText);
    ATSPIFREE(m_AtspiEditableText);

    // 5.获取输入框句柄并缓存
    m_AtspiText = atspi_accessible_get_text_iface (event->source);
    if(!m_AtspiText)
    {
        m_lastError = "---init m_AtspiText failed---";
        if(ENABLE_ATSPI_DEBUG) qDebug() << m_lastError;
        goto end ;
    }

    m_AtspiEditableText = atspi_accessible_get_editable_text_iface(event->source);
    if(!m_AtspiEditableText)
    {
        m_lastError = "---init m_AtspiEditableText failed---";
        if(ENABLE_ATSPI_DEBUG) qDebug() << m_lastError;
        goto end ;
    }

    widgetRoleNameTmp = atspi_accessible_get_localized_role_name (event->source,nullptr);
    widgetRoleName = widgetRoleNameTmp;
    g_free(widgetRoleNameTmp);
    if(ENABLE_ATSPI_DEBUG) qDebug() << " ---toolkit_name:" << widgetToolkitName << " windgetRoleName:"<< widgetRoleName ;

    // 6.区分gtk框架程序，动态修正时处理不同
    if(widgetToolkitName.compare("gtk") == 0)
    {
        m_toolkitIsGtk = true;
    }

    m_textFouceSuccessPid = m_seletedProgramPid;

end:
    m_atspiDebugInfo += "app(" + QString(appName) + ") toolkit(" + widgetToolkitName + ") roleName(" + widgetRoleName + ")";
    if(ENABLE_ATSPI_DEBUG) qDebug() << " ---------   End init --------- ";
}


void AtspiDesktop::listenerRegister()
{
    qDebug() << "enter listener_register";
    int atspi_status = atspi_init ();
    qDebug() << "Starting [atspi status " + QString::number(atspi_status) + "]";
    AtspiEventListener *listener = atspi_event_listener_new (onEvent, this,nullptr);
    if(listener)
    {
        qDebug() << "enter focused:" << atspi_event_listener_register (listener, "object:state-changed:focused", nullptr);
        qDebug() << "enter deactive:" << atspi_event_listener_register (listener, "window:", nullptr);
    }
    else
    {
        qDebug() << "enter focused is nullptr";
    }

    atspi_event_main ();
    qDebug() << "end listener_register atspi_event_main";
}

void AtspiDesktop::listenerUnRegister()
{
    qDebug() << "enter atspi_event_quit atspi_exit ";

    atspi_event_quit ();

    atspi_exit ();

    qDebug() << "end  atspi_event_quit atspi_exit ";
}

void AtspiDesktop::run()
{
#ifdef AUTO_ENABLE_ACCESSIBLE
    char cmdGsettings[128] = "gsettings set org.gnome.desktop.interface toolkit-accessibility true";
    if(0 == system(cmdGsettings))
    {
        qDebug() << "exec:[gsettings set org.gnome.desktop.interface toolkit-accessibility true]success";
    }
    else
    {
        qDebug() << "exec gsettings failed";
    }
#endif
    if (Util::isAccessibleEnable())
        listenerRegister();
}

AtspiDesktop* AtspiDesktop::getInstance()
{
    static AtspiDesktop obj;
    return &obj;
}

bool AtspiDesktop::isCaptureTextSuccess()
{
    if(!m_AtspiText || m_seletedProgramPid != m_textFouceSuccessPid)
    {
        qDebug() << "m_AtspiText is nullptr or pid error:" << m_seletedProgramPid << m_textFouceSuccessPid;
        return false;
    }
    return true;
}
QString AtspiDesktop::getSelectedContext()
{
    if(!m_AtspiText || m_seletedProgramPid != m_textFouceSuccessPid)
    {
        qDebug() << "m_AtspiText is nullptr or pid error:" << m_seletedProgramPid << m_textFouceSuccessPid;
        return nullptr;
    }

    AtspiRange *atspiSelectRange = atspi_text_get_selection(m_AtspiText, 0, nullptr);
    qDebug() << "select pos:" << atspiSelectRange->start_offset << atspiSelectRange->end_offset;
    if(atspiSelectRange->start_offset == atspiSelectRange->end_offset)
    {
        return nullptr;
    }
    return QString(atspi_text_get_text(m_AtspiText, atspiSelectRange->start_offset, atspiSelectRange->end_offset, nullptr));
}

int AtspiDesktop::getSelectedProgramPId()
{
    return m_seletedProgramPid;
}

void AtspiDesktop::processVectorAtspiEvent(AtspiDesktop * thisObj)
{
    qDebug() << "~~~enter" << __FUNCTION__;
    AtspiEvent *tmpEvent = nullptr;
    while(thisObj->m_loop)
    {
        if (!thisObj->m_vecAtspiEvent.empty()) {
            QMutexLocker lock(&thisObj->m_mutex);
            tmpEvent = thisObj->m_vecAtspiEvent.back();
            thisObj->m_vecAtspiEvent.pop_back();
            while (true)
            {
                if(thisObj->m_vecAtspiEvent.empty()) {
                    break;
                }
                AtspiEvent *tmpEventdeleter = thisObj->m_vecAtspiEvent.front();
                atspiEventFree(tmpEventdeleter);
                thisObj->m_vecAtspiEvent.pop_front();
            }
        } else {
            usleep(20*1000);
            continue;
        }
        if(strcmp(tmpEvent->type,"object:state-changed:focused") == 0 && (!tmpEvent->detail1 || !isEditable(tmpEvent->source))) {
            atspiEventFree(tmpEvent);
            continue;
        }

        thisObj->processEvent(tmpEvent);
        if ( nullptr != tmpEvent) {
            atspiEventFree(tmpEvent);
            tmpEvent = nullptr;
        }
    }
    qDebug() << "~~~end" << __FUNCTION__;
}
