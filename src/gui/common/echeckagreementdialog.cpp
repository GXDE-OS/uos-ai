#include "echeckagreementdialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "gui/gutils.h"
#include "utils/util.h"

#include <QLoggingCategory>

DCORE_USE_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

using namespace uos_ai;
ECheckAgreementDialog::ECheckAgreementDialog(QWidget *parent) :  DAbstractDialog(parent)
{
    initUI();
    initConnect();
    adjustSize();
}

void ECheckAgreementDialog::onCheckBoxStateChanged(int state)
{
    qCInfo(logAIGUI) << "The user has checked " << (state == Qt::Checked);
    m_pConfiemButton->setEnabled(state == Qt::Checked);
}

void ECheckAgreementDialog::onCancelButtonClicked()
{
    qCDebug(logAIGUI) << "The user has withdrawn consent to the tripartite MCP server agreement." ;
    this->reject();
}

void ECheckAgreementDialog::onConfirmButtonClicked()
{
    qCDebug(logAIGUI) << "The user has accepted the tripartite MCP server agreement." ;
    DbWrapper::localDbWrapper().updateThirdPartyMcpAgreement(true);
    this->accept();
}

void ECheckAgreementDialog::onUpdateSystemFont(const QFont &)
{
    qCDebug(logAIGUI) << "System font changed, adjusting dialog size.";
    this->adjustSize();
}

void ECheckAgreementDialog::initUI()
{
    setFixedWidth(380);
    setMinimumHeight(250);

    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setIcon(QIcon(":/assets/images/warning.svg"));
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    //标题
    auto titlelabel = new DLabel;
    titlelabel->setWordWrap(true);
    titlelabel->setForegroundRole(QPalette::Text);
    //    titlelabel->setMinimumWidth(360);
    titlelabel->setText(tr("Enabling MCP Server Features"));
    DFontSizeManager::instance()->bind(titlelabel, DFontSizeManager::T6, QFont::Medium);
    auto titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->addStretch();
    titleLayout->addWidget(titlelabel);
    titleLayout->addStretch();
    titleLayout->setAlignment(Qt::AlignVCenter);

    //提示信息
    auto textlabel = new DLabel;
    textlabel->setWordWrap(true);
    textlabel->setForegroundRole(QPalette::Text);
    // 设置大小策略允许垂直扩展
    textlabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    textlabel->setMinimumHeight(80);
    // 设置最大宽度以便正确计算换行后的高度
    textlabel->setMaximumWidth(340); // 留出一些边距
    textlabel->setAlignment(Qt::AlignCenter);
    textlabel->setText(tr("Some third-party MCP server features carry certain risks. Please use them with caution. If you enable this service, a built-in tool will detect and automatically download necessary dependencies. This download process will incur data charges. Please be aware of these risks and proceed with caution."));
    DFontSizeManager::instance()->bind(textlabel, DFontSizeManager::T6, QFont::Normal);
    auto textLayout = new QHBoxLayout();
    textLayout->setContentsMargins(20, 0, 20, 0);
    textLayout->addStretch();
    textLayout->addWidget(textlabel);
    textLayout->addStretch();
    textLayout->setAlignment(Qt::AlignVCenter);

    //单选框
    m_pCheckBox = new DCheckBox;
    m_pCheckBox->setFixedSize(QSize(22, 22));
    auto label = new DLabel;
    label->setContentsMargins(0, 0, 0, 3);
    label->setWordWrap(true);
    label->setForegroundRole(QPalette::Text);
    label->setText(tr("I have understood and agree to use this service"));
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T6, QFont::Medium);
    auto checkBoxLayout = new QHBoxLayout();
    checkBoxLayout->setContentsMargins(0, 0, 0, 0);
    checkBoxLayout->addStretch();
    // 确保复选框和标签都垂直居中对齐
    checkBoxLayout->addWidget(m_pCheckBox, 0, Qt::AlignTop);  //复选框和第一行文案对齐
    checkBoxLayout->setSpacing(8);
    checkBoxLayout->addWidget(label, 0, Qt::AlignVCenter);
    checkBoxLayout->addStretch();
    checkBoxLayout->setAlignment(Qt::AlignCenter);

    // 按钮区域
    m_pCancelButton = new DPushButton(tr("Cancel"));
    m_pCancelButton->setFixedWidth(170);
    m_pConfiemButton = new DSuggestButton(tr("Confirm"));
    m_pConfiemButton->setFixedWidth(170);
    m_pConfiemButton->setEnabled(false);

    // 创建分割线
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setFixedWidth(1);
    separator->setFixedHeight(28);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(10, 0, 10, 0);
    buttonLayout->addWidget(m_pCancelButton);
    buttonLayout->addSpacing(9);
    buttonLayout->addWidget(separator);
    buttonLayout->addSpacing(8);
    buttonLayout->addWidget(m_pConfiemButton);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(titleBar);
    layout->addSpacing(0);
    layout->addLayout(titleLayout, Qt::AlignCenter);
    layout->addSpacing(5);
    layout->addLayout(textLayout, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addLayout(checkBoxLayout, Qt::AlignCenter);
    layout->addSpacing(10);
    layout->addLayout(buttonLayout, Qt::AlignCenter);
    layout->addSpacing(10);

    this->setLayout(layout);
}

void ECheckAgreementDialog::initConnect()
{
    connect(m_pCheckBox, &DCheckBox::stateChanged, this, &ECheckAgreementDialog::onCheckBoxStateChanged);
    connect(m_pCancelButton, &DPushButton::clicked, this, &ECheckAgreementDialog::onCancelButtonClicked);
    connect(m_pConfiemButton, &DPushButton::clicked, this, &ECheckAgreementDialog::onConfirmButtonClicked);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
}
