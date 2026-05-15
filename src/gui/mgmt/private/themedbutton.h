#ifndef THEMEDBUTTON_H
#define THEMEDBUTTON_H

#include <DPushButton>
#include <DGuiApplicationHelper>

namespace uos_ai {
class ThemedButton : public DTK_WIDGET_NAMESPACE::DPushButton
{
    Q_OBJECT
public:
    enum ButtonStyle {
        Default,    // 默认样式，有文字颜色变化
        Plain       // 简洁样式，不改变文字颜色
    };

    explicit ThemedButton(QWidget *parent = nullptr);
    explicit ThemedButton(const QString &text, QWidget *parent = nullptr);
    ~ThemedButton() override;

    void setButtonStyle(ButtonStyle style);
    ButtonStyle buttonStyle() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void init();
    void updateStyle();

private:
    ButtonStyle m_buttonStyle = Default;
};
}
#endif // THEMEDBUTTON_H
