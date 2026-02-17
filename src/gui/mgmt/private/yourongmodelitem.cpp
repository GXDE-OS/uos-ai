#include "yourongmodelitem.h"

#include <QDebug>
#include <QLoggingCategory>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

YouRongModelItem::YouRongModelItem(bool small, DTK_WIDGET_NAMESPACE::DWidget *parent)
    : ModelScopeItem(small ? ItemInfo::yourongv1_1_5B() : ItemInfo::yourongv1_7B(), parent)
{
    // 可以添加额外的初始化逻辑
}

YouRongModelItem::~YouRongModelItem()
{
    // 可以添加额外的析构逻辑
}

void YouRongModelItem::initUI()
{
    ModelScopeItem::initUI();
    // 可以添加额外的界面初始化逻辑
}

void YouRongModelItem::onInstall()
{
    qCInfo(logAIGUI) << "Starting YouRong model installation";
    ModelScopeItem::onInstall();
    // 可以添加额外的安装逻辑
}
