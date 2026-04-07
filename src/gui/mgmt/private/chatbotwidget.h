#ifndef CHATBOTWIDGET_H
#define CHATBOTWIDGET_H

#include "uosai_global.h"

#include <DWidget>
#include <DBackgroundGroup>
#include <DSwitchButton>
#include <DLabel>
#include <QToolButton>

#include <QJsonObject>
#include <QMap>

DWIDGET_USE_NAMESPACE

class ThemedLable;
namespace uos_ai { namespace chatbot { class ChatBotService; } }

namespace uos_ai {

// ──────────────────────────────────────────────
// Helper: single platform row widget
// ──────────────────────────────────────────────
class PlatformRowWidget : public DWidget
{
    Q_OBJECT
public:
    explicit PlatformRowWidget(const QString &name, const QString &key, DWidget *parent = nullptr);

    DSwitchButton *sw() const { return m_sw; }
    void setChecked(bool checked);

Q_SIGNALS:
    void platformEnableChanged(const QString &key, bool enabled);
    void configureClicked(const QString &key);

private:
    QString        m_key;
    DLabel        *m_nameLabel = nullptr;
    DSwitchButton *m_sw        = nullptr;
    QToolButton   *m_editBtn   = nullptr;
};

// ──────────────────────────────────────────────
// ChatBotWidget
// ──────────────────────────────────────────────
class ChatBotWidget : public DWidget
{
    Q_OBJECT

public:
    explicit ChatBotWidget(DWidget *parent = nullptr);

    QString getTitleName();

private Q_SLOTS:
    void onThemeTypeChanged();
    void onMainEnableChanged(bool enabled);
    void onPlatformEnableChanged(const QString &key, bool enabled);
    void onConfigureClicked(const QString &key);
    void onServiceConfigChanged(const QJsonObject &config);

private:
    void initUI();
    void loadFromService();
    void saveToService();

    DBackgroundGroup *createPlatformGroup();
    void updatePlatformGroupVisible(bool enabled);

private:
    ThemedLable      *m_titleLabel    = nullptr;
    DSwitchButton    *m_mainSwitch    = nullptr;
    DBackgroundGroup *m_mainGroup     = nullptr;

    struct PlatformRow {
        DSwitchButton    *sw  = nullptr;
        PlatformRowWidget *row = nullptr;
    };
    QMap<QString, PlatformRow> m_platformRows;

    QJsonObject    m_config;
    uos_ai::chatbot::ChatBotService *m_service = nullptr;
};

} // namespace uos_ai

#endif // CHATBOTWIDGET_H
