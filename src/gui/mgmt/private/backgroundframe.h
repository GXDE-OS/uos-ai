#ifndef BACKGROUNDFRAME_H
#define BACKGROUNDFRAME_H

#include <DGuiApplicationHelper>
#include <DFrame>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

class BackgroundFrame : public DFrame
{
    Q_OBJECT
public:
    explicit BackgroundFrame(QWidget *parent = nullptr);
    ~BackgroundFrame() override;

private slots:
    void updateSystemTheme(const DGuiApplicationHelper::ColorType &themeType);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void paintLight();
    void paintDark();

private:
    bool m_isDarkTheme;
};

#endif // BACKGROUNDFRAME_H

