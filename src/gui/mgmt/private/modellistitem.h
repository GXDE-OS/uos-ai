#ifndef MODELLISTITEM_H
#define MODELLISTITEM_H

#include "serverdefs.h"

#include <DWidget>

DWIDGET_USE_NAMESPACE

class OperatingLineWidget;

class ModelListItem : public DWidget
{
    Q_OBJECT

public:
    explicit ModelListItem(const LLMServerProxy &data, DWidget *parent = nullptr);
    // 其他成员函数和数据成员
    void setEditMode(bool);

signals:
    void signalDeleteItem(const LLMServerProxy &data);

private slots:
    void onEditButtonClicked();
    void onDeleteButtonClicked();

private:
    void initUI();
    void initConnect();

private:
    OperatingLineWidget *m_pWidget = nullptr;
    LLMServerProxy m_data;
};

#endif // MODELLISTITEM_H
