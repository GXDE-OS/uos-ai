#include "useragreementdialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "themedlable.h"
#include "wrapcheckbox.h"
#include "utils/esystemcontext.h"
#include "private/echatwndmanager.h"

#include <DTitlebar>
#include <DFontSizeManager>
#include <DPushButton>
#include <DHorizontalLine>
#include <DLabel>
#include <DCheckBox>
#include <DIconButton>
#include <DArrowRectangle>
#include <DPalette>

#include <QVBoxLayout>
#include <QTimer>
#include <QTextEdit>
#include <QScrollArea>
#include <QApplication>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static constexpr char info[] = "uos-ai-assistant_info";

UserAgreementDialog::UserAgreementDialog(DWidget *parent):
    DAbstractDialog(parent)
{
    EWndManager()->registeWindow(this);
    initUI();
}

void UserAgreementDialog::initUI()
{
    setFixedSize(520, 650);
    //标题栏
    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);

    ThemedLable *titleLable = new ThemedLable(tr("UOS AI User Agreement"));
    titleLable->setAlignment(Qt::AlignCenter);
    titleLable->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    titleBar->setCustomWidget(titleLable, true);
    DFontSizeManager::instance()->bind(titleLable, DFontSizeManager::T5, QFont::DemiBold);

    ThemedLable *label = new ThemedLable(getAgreementText());
    label->setPaletteColor(QPalette::Text, DPalette::TextTitle);
    label->setWordWrap(true);
    DFontSizeManager::instance()->bind(label, DFontSizeManager::T8, QFont::Normal);

    auto labelWidget = new DWidget;
    auto labelLayout = new QHBoxLayout(labelWidget);
    labelLayout->setContentsMargins(25, 0, 20, 0);
    labelLayout->addWidget(label);
    auto scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(labelWidget);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWindowFlags(Qt::FramelessWindowHint);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
    scrollArea->setLineWidth(0);
    DPalette pl2 = scrollArea->palette();
    pl2.setColor(QPalette::Window, Qt::transparent);
    scrollArea->setPalette(pl2);
    auto scrollLayout = new QHBoxLayout;
    scrollLayout->setContentsMargins(1, 0, 1, 0);
    scrollLayout->addWidget(scrollArea);

    auto pAgrCheckbox = new WrapCheckBox;
    pAgrCheckbox->setFixedWidth(468);
    pAgrCheckbox->setTextMaxWidth(438);
    pAgrCheckbox->setText(tr("I confirm that I am over 18 years old. I acknowledge and agree that the contents I send and receive via the Application are direct data exchanges with the large model service provider and have nothing to do with the Company."));
    pAgrCheckbox->setCheckState(Qt::Checked);
    pAgrCheckbox->setDisabled(true);

    auto agrLayout = new QHBoxLayout();
    agrLayout->setContentsMargins(20, 0, 20, 0);
    agrLayout->addWidget(pAgrCheckbox, 0, Qt::AlignTop | Qt::AlignLeft);
    agrLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 10);
    layout->setSpacing(0);
    layout->addWidget(titleBar);
    layout->addSpacing(10);
    layout->addLayout(scrollLayout);
    layout->addSpacing(10);
    layout->addLayout(agrLayout);
    layout->addSpacing(10);

    this->setLayout(layout);
}


DArrowRectangle *UserAgreementDialog::showArrowRectangle(DArrowRectangle::ArrowDirection direction)
{
    qCDebug(logAIGUI) << "Creating arrow rectangle with direction:" << direction;
    DArrowRectangle *pExpTips = nullptr;
    if (ESystemContext::isWayland())
        pExpTips = new DArrowRectangle(direction, DArrowRectangle::FloatWidget, this);
    else
        pExpTips = new DArrowRectangle(direction, DArrowRectangle::FloatWindow, this);
    pExpTips->setRadiusArrowStyleEnable(true);
    pExpTips->setRadius(16);
    QColor color;
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        color = QColor(255,255,255);
    else {
        color = QColor(0,0,0);
        color.setAlphaF(0.3);
    }
    pExpTips->setBackgroundColor(color);
    pExpTips->setMargin(15);

    auto pExpContent = new DLabel;
    pExpContent->setText(tr("I agree to participate in the user experience plan of the Application, and authorize your company to collect the contents I send while using the Application, the time of sending, the type of requested large model ，the specific application and whether the text generated the image successfully, so as to improve the service quality and enhance the operation experience. (If you refuse to provide the above information, it will not affect your normal use of the Application.)"));
    pExpContent->setWordWrap(true);
    pExpContent->setFixedWidth(250);
    QPalette pl = pExpContent->palette();
    pl.setColor(QPalette::Text, DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Text));
    pExpContent->setPalette(pl);
    pExpContent->setForegroundRole(QPalette::Text);
    pExpContent->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(pExpContent, DFontSizeManager::T8, QFont::Medium);
    pExpTips->setContent(pExpContent);

    return pExpTips;
}

QString UserAgreementDialog::getAgreementText()
{
    QString content;
    // 打开资源文件
    QFile file(QLocale::Chinese == QLocale::system().language() && QLocale::SimplifiedChineseScript == QLocale::system().script() ? ":/assets/useragreement/chinese.txt" : ":/assets/useragreement/english.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 读取文本内容
        content = file.readAll();
        file.close();
        qCDebug(logAIGUI) << "Agreement text loaded successfully";
    } else {
        qCWarning(logAIGUI) << "Failed to open agreement file:" << file.fileName();
    }
    return content;
}
