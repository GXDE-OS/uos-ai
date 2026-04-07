#ifndef CHATBOTPLATFORMDIALOG_H
#define CHATBOTPLATFORMDIALOG_H

#include "uosai_global.h"

#include <DAbstractDialog>
#include <DLineEdit>
#include <DPasswordEdit>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

#include <QJsonObject>

DWIDGET_USE_NAMESPACE

namespace uos_ai {

/**
 * @brief ChatBotPlatformDialog - 编辑单个 IM 平台的接入凭据
 *
 * 根据 platformKey（feishu / dingtalk / qq）显示不同标题和字段：
 *   - feishu:   App ID + App Secret
 *   - dingtalk: Client ID + Client Secret
 *   - qq:       App ID + Token
 */
class ChatBotPlatformDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit ChatBotPlatformDialog(const QString &platformKey, DWidget *parent = nullptr);

    void setConfig(const QJsonObject &cfg);
    QJsonObject config() const;

private:
    void initUI();
    void initConnect();
    void updateConfirmEnabled();
    void updateHelpLabel();

private:
    QString m_platformKey;

    // feishu & qq
    DPasswordEdit *m_field1Edit = nullptr; // app_id / client_id
    DPasswordEdit *m_field2Edit = nullptr; // app_secret / client_secret / token
    // dingtalk only (optional)
    DLineEdit     *m_field3Edit = nullptr; // card_template_id

    DLabel         *m_helpLabel  = nullptr;
    DPushButton    *m_cancelBtn  = nullptr;
    DSuggestButton *m_confirmBtn = nullptr;
};

} // namespace uos_ai

#endif // CHATBOTPLATFORMDIALOG_H
