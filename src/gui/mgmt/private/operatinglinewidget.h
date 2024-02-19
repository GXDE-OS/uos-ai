#ifndef OPERATINGLINEWIDGET_H
#define OPERATINGLINEWIDGET_H

#include <DWidget>
#include <DLabel>
#include <DIconButton>
#include <DFontSizeManager>

DWIDGET_USE_NAMESPACE

class IconButtonEx;

class OperatingLineWidget: public DWidget
{
    Q_OBJECT

public:
    explicit OperatingLineWidget(DWidget *parent = nullptr);

    // 其他成员函数和数据成员
    void setEditMode(bool);
    void setName(const QString &);

    void setEditText(const QString &);
    void setEditFont(DFontSizeManager::SizeType type, int weight);
    void setEditIconSize(const QSize &);
    void setEditHighlight(bool);
    void setEditSpacing(int);

    void setModelShow(bool);

    void setInterruptFilter(bool interrupt);
signals:
    void signalDeleteButtonClicked();
    void signalNotDeleteButtonClicked();

private:
    void initUI();
    void initConnect();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    DIconButton *m_pDeleteButton = nullptr;
    DIconButton *m_pModelButton = nullptr;
    IconButtonEx *m_pEditbutton = nullptr;

    DLabel *m_pName = nullptr;
    bool m_bInterrupt = false;
};

#endif // OPERATINGLINEWIDGET_H
