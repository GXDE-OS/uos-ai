#include "deepseekmodelitem.h"

#include <DDialog>
#include <DLabel>

#include <QHBoxLayout>
#include <QIcon>
#include <QDebug>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DeepSeekModelItem::DeepSeekModelItem(DTK_WIDGET_NAMESPACE::DWidget *parent)
    : ModelScopeItem(ItemInfo::deepseek_r1_1_5B(), parent)
{
    // 可以添加额外的初始化逻辑
}

DeepSeekModelItem::~DeepSeekModelItem()
{
    // 可以添加额外的析构逻辑
}

void DeepSeekModelItem::initUI()
{
    ModelScopeItem::initUI();
    // 可以添加额外的界面初始化逻辑
}

void DeepSeekModelItem::onInstall()
{
    qCInfo(logAIGUI) << "DeepSeekModelItem onInstall triggered.";
    // 注意事项
    // DeepSeek本地模型对电脑配置有一定要求，低于推荐配置体验会有影响。
    // 暂不安装 确定安装
    // 查看推荐配置
    // 推荐配置
    // CPU：高性能多核CPU（如Intel i5/i7或AMD Ryzen 5/7系列），主频3.0 GHz以上
    // GPU：显存4GB以上
    // 内存：8GB及以上
    // 硬盘：固态硬盘（SSD）并预留5GB以上空间
    DDialog dlg(this);
    dlg.setIcon(QIcon(":/assets/images/warning.svg"));
    dlg.setMinimumWidth(380);
    dlg.setTitle(tr("Precautions"));
    dlg.setMessage(tr("The DeepSeek local model has certain requirements for computer configuration. If the configuration is lower than the recommended one, the experience will be affected."));

    DWidget *recommend = new DWidget(&dlg);
    QHBoxLayout *recommendLayout = new QHBoxLayout(recommend);
    DLabel *label = new DLabel(recommend);
    label->setText(tr("Recommended configuration"));
    QString tip(tr("<b>Recommended Configuration</b><br><b>CPU: </b>High-performance multi-core CPU (such as Intel i5/i7 or AMD Ryzen 5/7 series), with a clock speed of above 3.0 GHz.<br><b>GPU: </b>With a video memory of above 4GB.<br><b>Memory: </b>8GB and above.<br><b>Hard Drive: </b>Solid State Drive (SSD) with at least 5GB of free space reserved."));
    label->setToolTip(tip);
    DLabel *label2 = new DLabel(recommend);
    label2->setPixmap(QIcon::fromTheme("uos-ai-assistant_tips").pixmap(20, 20));
    label2->setToolTip(tip);
    recommendLayout->addWidget(label);
    recommendLayout->addWidget(label2);
    dlg.addContent(recommend, Qt::AlignCenter);

    dlg.addButton(tr("Install later"), false, DDialog::ButtonNormal);
    dlg.addButton(tr("Confirm installation"), true, DDialog::ButtonRecommend);

    if (DDialog::Accepted != dlg.exec()) {
        qCInfo(logAIGUI) << "User cancelled DeepSeek model installation.";
        return;
    }

    qCInfo(logAIGUI) << "User confirmed DeepSeek model installation.";
    ModelScopeItem::onInstall();
}
