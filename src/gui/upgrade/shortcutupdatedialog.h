#ifndef SHORTCUTUPDATEDIALOG_H
#define SHORTCUTUPDATEDIALOG_H
#include "uosai_global.h"

#include <DAbstractDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>
#include <DGuiApplicationHelper>

class ThemedLable;

namespace uos_ai {
class ShortcutUpdateDialog : public DTK_WIDGET_NAMESPACE::DAbstractDialog
{
    Q_OBJECT
public:
    ShortcutUpdateDialog(QWidget *parent = nullptr);
    static bool checkAndShow(QWidget *parent = nullptr);
    static int needUpdatePrompt();
    static bool isShortcutRight();

private slots:
    void onUpdateSystemTheme(const DTK_GUI_NAMESPACE::DGuiApplicationHelper::ColorType &);
    void onUpdateSystemFont(const QFont &);

private:
    void initUI();
    void initConnect();
    void resetLinkColor();
    void changeDisplayImage();
    
private:
    DTK_WIDGET_NAMESPACE::DLabel *m_imageLabel;
    DTK_WIDGET_NAMESPACE::DLabel *m_textLabel;
    DTK_WIDGET_NAMESPACE::DSuggestButton *m_confirmBtn;
    ThemedLable *m_tipLabel;
    int m_currentPromptCount; // 记录当前提示次数
};
}

#endif // ShortcutUpdateDialog_H
