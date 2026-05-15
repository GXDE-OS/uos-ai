#ifndef CHATBOTPLATFORMDIALOG_H
#define CHATBOTPLATFORMDIALOG_H

#include <DAbstractDialog>
#include <DLineEdit>
#include <DPasswordEdit>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

#include <QJsonObject>
#include <QVector>

DWIDGET_USE_NAMESPACE

namespace uos_ai {

/**
 * @brief ChatBotPlatformDialog - 编辑单个 IM 平台的接入凭据
 *
 * 根据 platformKey（feishu / dingtalk / qq / telegram / discord）显示不同标题和字段：
 *   - feishu:   App ID + App Secret
 *   - dingtalk: Client ID + Client Secret + Card Template ID(optional)
 *   - qq:       App ID + Token
 *   - telegram: Bot Token + API Base(optional)
 *   - discord:  Bot Token + Application ID + Guild ID(optional)
 */
class ChatBotPlatformDialog : public DAbstractDialog
{
    Q_OBJECT

public:
    explicit ChatBotPlatformDialog(const QString &platformKey, DWidget *parent = nullptr);

    void setConfig(const QJsonObject &cfg);
    QJsonObject config() const;

private:
    struct FieldConfig {
        QString key;
        QString label;
        bool required = false;
        bool password = false;
    };

    struct FieldWidgets {
        FieldConfig config;
        DLabel *label = nullptr;
        DLineEdit *edit = nullptr;
    };

    void initUI();
    void initConnect();
    void updateConfirmEnabled();
    void updateHelpLabel();
    QVector<FieldConfig> fieldConfigs() const;

private:
    QString m_platformKey;
    QVector<FieldWidgets> m_fields;

    DLabel         *m_helpLabel  = nullptr;
    DPushButton    *m_cancelBtn  = nullptr;
    DSuggestButton *m_confirmBtn = nullptr;
};

} // namespace uos_ai

#endif // CHATBOTPLATFORMDIALOG_H
