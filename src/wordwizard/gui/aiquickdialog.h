#ifndef AIQUICKDIALOG_H
#define AIQUICKDIALOG_H

#include "gui/gutils.h"
#include "../wordwizard.h"
#include "../private/transbutton.h"
#include "../private/querytextedit.h"
#include <QVariant>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMutex>
#include <QMovie>

#include <DAbstractDialog>
#include <DWidget>
#include <DLabel>
#include <DMenu>
#include <DLineEdit>
#include <DPlainTextEdit>
#include <DPushButton>
#include <DWindowCloseButton>
#include <DComboBox>
#include <DTitlebar>

namespace uos_ai {

class CustomDMenu;
class SessionManager;
class ConversationRecord;

class AiQuickDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT

public:
    enum {
        ERROR_TYPE_NONE = 0,
        ERROR_TYPE_NO_MODEL, // 没模型
        ERROR_TYPE_ACCOUNT_LIMIT, // 没额度
        ERROR_TYPE_ACCOUNT_EXPIRED, // 账号过期
        ERROR_TYPE_ACCOUNT_INVALID, // 账号无效
        ERROR_TYPE_NETWORK_ERROR, // 没网
        ERROR_TYPE_OTHER, // other
    };

    enum {
        LANGUAGE_TYPE_SIMPLIFIED_CHINESE = 0, // 简体中文
        LANGUAGE_TYPE_TRADITIONAL_CHINESE, // 繁体中文
        LANGUAGE_TYPE_TIBETAN, // 藏语
        LANGUAGE_TYPE_ENGLISH, // 英语
        LANGUAGE_TYPE_JAPANESE, // 日语
        LANGUAGE_TYPE_GERMAN, // 德语
        LANGUAGE_TYPE_SPANISH, // 西班牙语
        LANGUAGE_TYPE_FRENCH, // 法语
        LANGUAGE_TYPE_ITALIAN, // 意大利语
        LANGUAGE_TYPE_KOREAN, // 韩语
        LANGUAGE_TYPE_MALAY, // 马来语
        LANGUAGE_TYPE_PORTUGUESE, // 葡萄牙语
        LANGUAGE_TYPE_RUSSIAN, // 俄语
        LANGUAGE_TYPE_THAI, // 泰语
        LANGUAGE_TYPE_VIETNAMESE, // 越南语
    };

    static bool almostAllEnglish(QString text);
    explicit AiQuickDialog(QObject *parent = nullptr);
    ~AiQuickDialog() override;

    /**
     * @brief 设置快捷面板内容
     * @param type 处理类别
     * @param query 文本
     * @param pos 显示位置，上方中心点
     */
    void setQuery(int type, QString query, QPoint pos, bool isCustom, const QString &imagePath = "");
    void setQuerySepHeight(int height);
    void showToast(const QString &message);
    void copyText(const QString &text);

public slots:
    virtual void reject() override;
    void onModelReply(int op, QString reply, int err);
    void onUpdateSystemTheme();
    void onFontChanged(const QFont &font);
    void onWritableStateChanged(bool isTrue);
    void onFocusIn() { onWritableStateChanged(true); }
    void onFocusOut() { onWritableStateChanged(false); }

private:
    void initUi();
    void initConnect();

    /**
     * @brief 设置当前处理类型
     * @param type
     */
    void setQueryType(int type);

    /**
     * @brief 设置当前处理类型，针对自定义功能
     * @param func
     */
    void setCustomQueryType(const uos_ai::CustomFunction &func);

    /**
     * @brief 发起AI请求
     */
    void sendAiRequst();

    /**
     * @brief 平滑输出生成内容
     */
    void putAiReply(QString reply = "");

    /**
     * @brief 平滑输出生成内容
     */
    void smoothEachWord();

    /**
     * @brief 继续对话
     */
    void continueDialog();

    /**
     * @brief 文本编辑器的高度取决于请求内容的长度
     */
    void adjustQueryTextEditSize() {}

    /**
     * @brief 文本编辑器的高度取决于生成内容的长度
     */
    void adjustReplyTextEditSize();

    /**
     * @brief 对话框不可超出屏幕范围
     */
    void adjustDialogPosition();

    /**
     * @brief 同步当前UOS AI助手账号信息
     */
    void syncLlmAccount();

    /**
     * @brief 处理异常情况，如无模型、账号过期
     */
    void handleError();

    /**
     * @brief 异步调整布局尺寸，某些场景同步会不生效
     */
    void asyncAdjustSize(int ms = 0);

    /**
     * @brief 未生成完成，或账号异常，一些控件不可用
     * @param isTrue
     */
    void enableReplyFunBt(bool isTrue);

    /**
     * @brief 生成过程中，不可用控件部分要隐藏
     * @param isTrue
     */
    void setReplyPartVisible(bool isTrue);

    /**
     * @brief 判断是否支持朗读
     * @return
     */
    bool isReadAloudSupported() const;

    void updateKnowledgeActionEnabled();
    /**
     * @brief 启动新process进行ocr识别，将结果发送给大模型
     */
    void runOCRProcessByPath(int type, bool isCustom, const QString &imagePath);
    /**
     * @brief sendWordWizardRequest: Responding to user conversations
     * @param conversation
     * @param stream: Conversation flow switch.
     * @param temperature: A parameter that returns randomness, where a higher value indicates higher randomness.
     */
    QString sendWordWizardRequest(const QString &llmId, const QString &conversation, qreal temperature, bool stream = false);

private:
    /**
     * @brief Convert request to conversation record
     */
    QSharedPointer<uos_ai::ConversationRecord> convertRequestion(const QString &id, const QString &json);

    /**
     * @brief Handle session events
     */
    void onEvent(int event, const QString &id, const QString &json);

private slots:
    void onBtClicked();
    void onMenuTriggered(QAction *action);
    void onOpenConfigDialog(const QString& link);

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QObject *m_parent = nullptr;

    DGuiApplicationHelper::ColorType m_themeType = DGuiApplicationHelper::LightType;

    int m_queryType    = 0;
    int m_queryTypeBak = 0;
    QString m_query;
    QPoint m_pos;

    QString m_reply;
    QString m_replyBak;

    uos_ai::QueryTextEdit *m_queryTextEdit = nullptr;

    DTK_WIDGET_NAMESPACE::DPushButton *m_logoBt           = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_titleBt          = nullptr;
    DTK_WIDGET_NAMESPACE::DTitlebar *m_titleBar           = nullptr;
    DTK_WIDGET_NAMESPACE::DWindowCloseButton *m_closeBt   = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_querySep             = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_queryHSep            = nullptr;
    QHBoxLayout *m_queryLayout      = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_longArrowLabel        = nullptr;
    QTextEdit *m_replyTextEdit      = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_errorInfoLabel        = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_cancelBt         = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_inProgressLabel       = nullptr;
    QMovie *m_inProgressMovie       = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_inOCRLabel            = nullptr;
    QMovie *m_inOCRMovie            = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_readBt           = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_replaceBt        = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_replySep1            = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_againBt          = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_copyBt           = nullptr;
    QHBoxLayout *m_replyFunLayout   = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_continueBt       = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_attentionLabel        = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_modelBt          = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_vSpace               = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_vSpace1              = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_vSpace2              = nullptr;
    int m_queryTextEditMax = 0;
    int m_replyTextEditMax = 0;
    volatile bool m_isReplyEditCursorEnd = true;
    // 当前是否是可写状态，如网页就是不可写
    volatile bool m_isWritable = true;
    volatile bool m_isQueryNeedShowTip = false;
    // 第一次使用填充功能，提醒用户重启输入法服务
    bool m_isFirstFill = false;

    // menu
    CustomDMenu *m_menu = nullptr;
    QAction *m_searchAction    = nullptr;
    QAction *m_explainAction   = nullptr;
    QAction *m_summarizeAction = nullptr;
    QAction *m_translateAction = nullptr;
    QAction *m_renewAction     = nullptr;
    QAction *m_extendAction    = nullptr;
    QAction *m_correctAction   = nullptr;
    QAction *m_polishAction   = nullptr;
    QAction *m_knowledgeAction   = nullptr;
    QString m_menuLabel;

    // AI model
    QString m_systemPrompt;
    QString m_modelId;
    QString m_modelName;
    QString m_modelInfo;
    QString m_modelIcon;
    QString m_modelIdBak;
    QString m_modelNameBak;
    QString m_modelIconBak;
    QString m_reqId;
    int m_curErr = 0;
    QString m_curErrInfo;
    int m_modelErrCode = 0;

    // cache
    QString m_replyCache;
    int m_replyCacheIdx = 0;
    QMutex m_replyCacheMutex;
    volatile bool m_isReplyEnd = false;

    bool m_dragging = false;
    QPoint m_dragStartPos;

    DTK_WIDGET_NAMESPACE::DComboBox *m_autoComboBox = nullptr;
    DTK_WIDGET_NAMESPACE::DComboBox *m_languageComboBox = nullptr;
    static QList<QString> kLanguageList;
    static int kTargetLanguageIdx;
    // 语言与提示词映射
    QList<QString> m_languagePromptList;
    // 是否第一次非中文简体、英文翻译
    static bool kIsFirstTranslate;

    // 自定义功能列表
    QList<CustomFunction> m_customFunctionList;
    QSet<QAction *> m_showActions;

    // Chat session management
    struct ChatTask {
        QString id;
        bool stream = false;
        QSharedPointer<ConversationRecord> record;
    };
    ChatTask m_chatTask;
    QSharedPointer<SessionManager> chatSession;
};

}
#endif // AIQUICKDIALOG_H
