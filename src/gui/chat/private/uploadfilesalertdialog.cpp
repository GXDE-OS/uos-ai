#include "uploadfilesalertdialog.h"

#include <DSuggestButton>
#include <DFontSizeManager>
#include <DVerticalLine>
#include <DTitlebar>

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

UploadFilesAlertDialog::UploadFilesAlertDialog(QWidget *parent)
{
    initUI();
}

void UploadFilesAlertDialog::showOnlineModelAlert(const QString &modelName)
{
    QLabel *titleLabel = new QLabel(this);
    titleLabel->setContentsMargins(28, 0, 28, 0);
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T6, QFont::Medium);
    titleLabel->setForegroundRole(QPalette::BrightText);
    titleLabel->setText(tr("The materials will be uploaded to the online model (%1) for analysis. Do you want to continue generating?").arg(modelName));

    QLabel *msgLabel1st = new QLabel(this);
    msgLabel1st->setContentsMargins(30, 0, 30, 0);
    msgLabel1st->setAttribute(Qt::WA_TransparentForMouseEvents);
    msgLabel1st->setWordWrap(true);
    msgLabel1st->setAlignment(Qt::AlignLeft);
    msgLabel1st->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    DFontSizeManager::instance()->bind(msgLabel1st, DFontSizeManager::T6, QFont::Normal);
    msgLabel1st->setText(tr("If you do not want local materials to be uploaded, you can perform the following operations in the input box before generating content again:"));

    QLabel *msgLabel2nd = new QLabel(this);
    msgLabel2nd->setContentsMargins(30, 0, 30, 0);
    msgLabel2nd->setAttribute(Qt::WA_TransparentForMouseEvents);
    msgLabel2nd->setWordWrap(true);
    msgLabel2nd->setAlignment(Qt::AlignLeft);
    msgLabel2nd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    DFontSizeManager::instance()->bind(msgLabel2nd, DFontSizeManager::T6, QFont::Normal);
    msgLabel2nd->setText(tr("1.Switch the model to a local model (e.g., DeepSeek-R1-1.5B) or a privately deployed model"));

    QLabel *msgLabel3rd = new QLabel(this);
    msgLabel3rd->setContentsMargins(30, 0, 30, 0);
    msgLabel3rd->setAttribute(Qt::WA_TransparentForMouseEvents);
    msgLabel3rd->setWordWrap(true);
    msgLabel3rd->setAlignment(Qt::AlignLeft);
    msgLabel3rd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    DFontSizeManager::instance()->bind(msgLabel3rd, DFontSizeManager::T6, QFont::Normal);
    msgLabel3rd->setText(tr("2.Set \"Online Search\" to the unselected state"));

    m_mainLayout->addWidget(titleLabel, 0, Qt::AlignTop);
    m_mainLayout->addSpacing(10);
    m_mainLayout->addWidget(msgLabel1st, 1, Qt::AlignTop);
    m_mainLayout->addSpacing(20);
    m_mainLayout->addWidget(msgLabel2nd, 1, Qt::AlignTop);
    m_mainLayout->addSpacing(5);
    m_mainLayout->addWidget(msgLabel3rd, 1, Qt::AlignTop);
    m_mainLayout->addSpacing(30);

    initButtons();

    // Force layout update after adding all widgets
    updateHeight();
}

void UploadFilesAlertDialog::showLocalModelAlert()
{
    QLabel *titleLabel = new QLabel(this);
    titleLabel->setContentsMargins(28, 0, 28, 0);
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    titleLabel->setWordWrap(true);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    DFontSizeManager::instance()->bind(titleLabel, DFontSizeManager::T6, QFont::Medium);
    titleLabel->setForegroundRole(QPalette::BrightText);
    titleLabel->setText(tr("The materials will be uploaded to the network for searching. Do you wish to continue generating?"));

    QLabel *msgLabel1st = new QLabel(this);
    msgLabel1st->setContentsMargins(30, 0, 30, 0);
    msgLabel1st->setAttribute(Qt::WA_TransparentForMouseEvents);
    msgLabel1st->setWordWrap(true);
    msgLabel1st->setAlignment(Qt::AlignLeft);
    msgLabel1st->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    DFontSizeManager::instance()->bind(msgLabel1st, DFontSizeManager::T6, QFont::Normal);
    msgLabel1st->setText(tr("If you do not want local materials to be uploaded, you can set \"Online Search\" to the unselected state in the input box before proceeding to generate content."));

    m_mainLayout->addWidget(titleLabel, 0, Qt::AlignTop);
    m_mainLayout->addSpacing(10);
    m_mainLayout->addWidget(msgLabel1st, 10, Qt::AlignTop);
    m_mainLayout->addSpacing(30);

    initButtons();

    // Force layout update after adding all widgets
    updateHeight();
}

void UploadFilesAlertDialog::changeEvent(QEvent *event)
{
    DAbstractDialog::changeEvent(event);

    if (event->type() == QEvent::FontChange) {
        // 字号变化时重新调整 dialog 高度
        updateHeight();
    }
}

void UploadFilesAlertDialog::initUI()
{
    setFixedWidth(500);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setIcon(QIcon(":/assets/images/warning.svg"));
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    m_mainLayout->addWidget(titleBar, 0, Qt::AlignTop);

    setLayout(m_mainLayout);
}

void UploadFilesAlertDialog::initButtons()
{
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(10, 0, 10, 10);
    buttonLayout->setSpacing(5);

    m_continueButton = new DSuggestButton(this);
    m_continueButton->setText(tr("Generating"));
    m_continueButton->setAttribute(Qt::WA_NoMousePropagation);

    m_cancelButton = new QPushButton(this);
    m_cancelButton->setText(tr("Cancel"));
    m_cancelButton->setAttribute(Qt::WA_NoMousePropagation);

    DVerticalLine *line = new DVerticalLine;
    line->setFixedHeight(30);

    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(line);
    buttonLayout->addWidget(m_continueButton);

    connect(m_cancelButton, &QPushButton::clicked, this, &UploadFilesAlertDialog::reject);
    connect(m_continueButton, &QPushButton::clicked, this, &UploadFilesAlertDialog::accept);

    m_mainLayout->addLayout(buttonLayout);
}

void UploadFilesAlertDialog::updateHeight()
{
    int width = size().width();
    int height = m_mainLayout->heightForWidth(width);

    setFixedHeight(height);
}
