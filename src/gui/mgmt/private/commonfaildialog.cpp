#include "commonfaildialog.h"
#include "networkdefs.h"
#include "backgroundframe.h"

#include <QHBoxLayout>

#include <DFontSizeManager>
#include <DPushButton>
#include <DTitlebar>
#include <DLabel>

static constexpr char WARNING_ICON[] = ":/assets/images/warning.svg";

CommonFailDialog::CommonFailDialog(DWidget *parent)
    : DAbstractDialog(parent)
{
    setFixedSize(380, 256);
    initUI();
}

void CommonFailDialog::initUI()
{
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setIcon(QIcon(WARNING_ICON));
    titleBar->setBackgroundTransparent(true);
    DLabel *label = new DLabel(tr("Error"));
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T6, QFont::Normal);
    titleBar->setCustomWidget(label);

    BackgroundFrame *frame = new BackgroundFrame(this);
    m_pPlainTextEdit = new DPlainTextEdit(frame);
    DFontSizeManager::instance()->bind(m_pPlainTextEdit, DFontSizeManager::T8);
    DPalette p = m_pPlainTextEdit->palette();
    p.setColor(QPalette::Base, Qt::transparent);
    m_pPlainTextEdit->setPalette(p);
    m_pPlainTextEdit->setReadOnly(true);
    m_pPlainTextEdit->setFrameShape(DFrame::NoFrame);

    QHBoxLayout *frameLayout = new QHBoxLayout(frame);
    frameLayout->addWidget(m_pPlainTextEdit);
    frameLayout->setSpacing(0);
    frameLayout->setContentsMargins(5, 5, 5, 5);

    QVBoxLayout *pContentLayout = new QVBoxLayout();
    pContentLayout->setContentsMargins(10, 0, 10, 0);
    pContentLayout->setSpacing(20);

    DPushButton *btn = new DPushButton(tr("OK", "button"), this);
    connect(btn, &DPushButton::clicked, this, &CommonFailDialog::accept);

    pContentLayout->addWidget(frame);
    pContentLayout->addWidget(btn, Qt::AlignCenter);

    QVBoxLayout *pMainLayout = new QVBoxLayout();
    pMainLayout->setSpacing(0);
    pMainLayout->addWidget(titleBar);
    pMainLayout->addLayout(pContentLayout);
    pMainLayout->setContentsMargins(0, 0, 0, 10);
    setLayout(pMainLayout);
}

void CommonFailDialog::setFailMsg(const QString &msg)
{
    m_pPlainTextEdit->setPlainText(msg);
}
