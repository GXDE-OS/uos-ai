#ifndef ICONBUTTONEX_H
#define ICONBUTTONEX_H

#include <DWidget>
#include <DFontSizeManager>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class IconButtonEx : public DWidget
{
    Q_OBJECT

public:
    explicit IconButtonEx(DWidget *parent = nullptr);
    explicit IconButtonEx(const QString text, DWidget *parent = nullptr);

    void setText(const QString &);
    void setFont(DFontSizeManager::SizeType type, int weight);
    void setIconSize(const QSize &);
    void setHighlight(bool);
    void setSpacing(int);
    void setInterruptFilter(bool);

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    DLabel *m_pLabel = nullptr;
    DLabel *m_pIcon = nullptr;
    bool m_bHighlight = false;
    bool m_bInterrupt = false;
    QSize m_iconSize;
};

#endif // ICONBUTTONEX_H
