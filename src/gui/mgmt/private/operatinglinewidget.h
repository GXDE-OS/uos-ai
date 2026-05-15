#ifndef OPERATINGLINEWIDGET_H
#define OPERATINGLINEWIDGET_H

#include "mgmtdefs.h"

#include <DWidget>
#include <DLabel>
#include <DIconButton>
#include <DFontSizeManager>

namespace uos_ai {

class IconButtonEx;
class OperatingLineWidget: public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:
    explicit OperatingLineWidget(DTK_WIDGET_NAMESPACE::DWidget *parent = nullptr);

    // 其他成员函数和数据成员
    void setEditMode(bool);
    void setName(const QString &);

    void setEditText(const QString &);
    void setEditFont(DTK_WIDGET_NAMESPACE::DFontSizeManager::SizeType type, int weight);
    void setEditIconSize(const QSize &);
    void setEditHighlight(bool);
    void setEditSpacing(int);

    void setModelShow(bool);

    void setInterruptFilter(bool interrupt);

    void setStatusIcon(const QString &iconName);
    void setTipsIcon(const QString &iconName);
    void setBookIcon();

    void setFileSize(qint64 bytes);
    qint64 fileSize() { return m_fileSize; }

    void setSpinnerVisible(bool visible);

    void setStatus(KnowledgeBaseProcessStatus status);
signals:
    void signalDeleteButtonClicked();
    void signalNotDeleteButtonClicked(QString objectname);

private:
    void initUI();
    void initConnect();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    DTK_WIDGET_NAMESPACE::DIconButton *m_pDeleteButton = nullptr;
    DTK_WIDGET_NAMESPACE::DIconButton *m_pModelButton = nullptr;
    IconButtonEx *m_pEditbutton = nullptr;

    DTK_WIDGET_NAMESPACE::DLabel *m_pName = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_pFileSize = nullptr;
    bool m_bInterrupt = false;
    qint64 m_fileSize = 0;
};
}
#endif // OPERATINGLINEWIDGET_H
