#ifndef AIWRITERDIALOG_H
#define AIWRITERDIALOG_H

#include "private/querytextedit.h"

#include <uosai_global.h>
#include <markdownhighlighter.h>

#include <DAbstractDialog>
#include <DWidget>
#include <DLabel>
#include <DLineEdit>
#include <DPlainTextEdit>
#include <DTextEdit>
#include <DPushButton>
#include <DTitlebar>
#include <DGuiApplicationHelper>
#include <DWidget>

#include <QMovie>
#include <QMutex>
#include <QDebug>

namespace uos_ai {

class FillTextButton;
class HintTextEdit;
class AiWriterDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(AiWriterDialog)

public:
    static AiWriterDialog &instance();
    void showDialog();
    void closeDialog();
    void setQuerySepHeight(int height);
    void showToast(const QString &message);
    void copyText(const QString &text);

public slots:
    void onModelReply(int op, QString reply, int err);
    void onFontChanged(const QFont &font);
    void onUosAiLlmAccountLstChanged();
    void onLlmAccountLstChanged(const QString &currentAccountId, const QString &accountLst);
    void onNetworkStateChanged(bool isOnline);

protected:
    void reject() override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private:
    explicit AiWriterDialog(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);
    ~AiWriterDialog() override;

    void initUi();
    void resetUi();
    void initConnect();
    void onUpdateSystemTheme(const Dtk::Gui::DGuiApplicationHelper::ColorType &);

    /**
     * @brief 同步当前UOS AI助手账号信息
     */
    void syncLlmAccount();

    /**
     * @brief 处理异常情况，如无模型、账号过期
     */
    void handleError();

    /**
     * @brief 发起AI请求
     * @param isAgain 是否是重新生成
     */
    void sendAiRequst(bool isAgain = false);

    /**
     * @brief 平滑输出生成内容
     */
    void putAiReply(QString reply = "");

    /**
     * @brief 平滑输出生成内容
     */
    void smoothEachWord();

    /**
     * @brief 填充至正文
     */
    void fillToText();

    /**
     * @brief 写文章等控件部分要隐藏
     * @param isTrue
     */
    void setArticleBtPartVisible(bool isTrue);

    /**
     * @brief 写文章等控件部分，清除选中的颜色变化
     */
    void clearArticleBtPressColor();

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
     * @brief 文本编辑器的高度取决于内容的长度
     */
    void adjustQueryEditSize();

    /**
     * @brief 文本编辑器的高度取决于内容的长度
     */
    void adjustReplyEditSize();

    /**
     * @brief 对话框初始位置
     */
    void adjustDialogInitPosition();

    /**
     * @brief 对话框不可超出屏幕范围
     */
    void adjustDialogPosition();

    /**
     * @brief 异步调整布局尺寸，某些场景同步会不生效
     */
    void asyncAdjustSize(int ms = 0);

private slots:
    void onBtClicked();
    void onOpenConfigDialog(const QString& link);
    void onTextChanged();
    void onQueryTextChanged(bool isActiveChange = false);

private:
    // 初始化完成的标志
    volatile bool m_isInitOk = false;

    Dtk::Gui::DGuiApplicationHelper::ColorType m_themeType = Dtk::Gui::DGuiApplicationHelper::LightType;

    uos_ai::QueryTextEdit *m_queryTextEdit = nullptr;

    MarkdownHighlighter *m_highlighter = nullptr;

    DTK_WIDGET_NAMESPACE::DPushButton *m_logoBt       = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_sendBt       = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_querySpace       = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_sendBt1      = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_sendVSpace       = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_querySep         = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_queryHSep        = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_queryHSepVSpace  = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_errorInfoLabel    = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_cancelBt     = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_inProgressLabel   = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_progressVSpace   = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_replaceBt    = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_replySep         = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_againBt      = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_copyBt       = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_attentionLabel    = nullptr;
    DTK_WIDGET_NAMESPACE::DPushButton *m_modelBt      = nullptr;

    FillTextButton *m_articleBt = nullptr;
    FillTextButton *m_outlineBt = nullptr;
    FillTextButton *m_notifyBt = nullptr;
    FillTextButton *m_reportBt = nullptr;
    FillTextButton *m_speechBt = nullptr;
    FillTextButton *m_summaryBt = nullptr;
    FillTextButton *m_publicBt = nullptr;
    FillTextButton *m_curBt = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_articleVSpace  = nullptr;
    DTK_WIDGET_NAMESPACE::DWidget *m_articleVSpace1 = nullptr;


    DTK_WIDGET_NAMESPACE::DWidget *m_vSpace           = nullptr;

    // DTextEdit更改不了背景色，此处使用QTextEdit
    HintTextEdit *m_queryEdit          = nullptr;
    QMovie *m_inProgressMovie       = nullptr;
    QTextEdit *m_replyEdit          = nullptr;

    int m_queryEditMax    = 0;
    int m_queryEditMin    = 0;
    int m_replyEditMax    = 0;
    // 用于取消button点击后，恢复原有布局
    int m_replyEditMaxBak = 0;
    volatile bool m_isReplyEditCursorEnd = true;

    // 是否是第一轮交互
    volatile bool m_isNeedResetUi = false;
    volatile bool m_isShow        = false;
    // 第一次使用填充功能，提醒用户重启输入法服务
    bool m_isFirstFill = false;

    QString m_query;
    QString m_reply;
    QString m_replyBak;

    // AI model
    QString m_systemPrompt;
    QString m_prompt;
    QString m_modelId;
    QString m_modelInfo;
    QString m_reqId;
    int m_curErr = 0;
    QString m_curErrInfo;

    // cache
    QString m_replyCache;
    int m_replyCacheIdx = 0;
    QMutex m_replyCacheMutex;
    volatile bool m_isReplyEnd = false;

    QPoint m_initPos;
};
}

#endif // AIWRITERDIALOG_H
