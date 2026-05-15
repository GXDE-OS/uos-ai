#ifndef BACKGROUNDFRAME_H
#define BACKGROUNDFRAME_H

#include <DGuiApplicationHelper>
#include <DFrame>

namespace uos_ai {

class BackgroundFrame : public DTK_WIDGET_NAMESPACE::DFrame
{
    Q_OBJECT
public:
    explicit BackgroundFrame(QWidget *parent = nullptr);
    ~BackgroundFrame() override;

private slots:
    void updateSystemTheme(const DTK_GUI_NAMESPACE::DGuiApplicationHelper::ColorType &themeType);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void paintLight();
    void paintDark();

private:
    bool m_isDarkTheme;
};


}

#endif // BACKGROUNDFRAME_H

