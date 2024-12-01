#include "useragreementdialog.h"
#include "dbwrapper.h"
#include "serverwrapper.h"
#include "themedlable.h"
#include "wrapcheckbox.h"

#include <QVBoxLayout>
#include <QTimer>
#include <QTextEdit>
#include <QScrollArea>
#include <QApplication>
#include <QDesktopWidget>

#include <DTitlebar>
#include <DFontSizeManager>
#include <DPushButton>
#include <DHorizontalLine>
#include <DLabel>
#include <DCheckBox>
#include <DIconButton>
#include <DArrowRectangle>
#include <DPalette>

static constexpr char info[] = "uos-ai-assistant_info";

UserAgreementDialog::UserAgreementDialog(DWidget *parent):
    DAbstractDialog(parent)
{
    initUI();
    initConnect();
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
    pl2.setColor(QPalette::Background, Qt::transparent);
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

    m_pExpCheckbox = new WrapCheckBox;
    m_pExpCheckbox->setTextMaxWidth(410);
    m_pExpCheckbox->setText(tr("I agree to participate in the user experience plan of the Application"));
    m_pExpCheckbox->setCheckState(DbWrapper::localDbWrapper().getUserExpState() > 0 ? Qt::Checked : Qt::Unchecked);

    m_pExpIcon = new DIconButton(static_cast<QStyle::StandardPixmap>(-1), this);
    m_pExpIcon->setFixedSize(QSize(20, 20));
    m_pExpIcon->setIcon(QIcon::fromTheme(info));
    m_pExpIcon->setIconSize(QSize(20, 20));
    m_pExpIcon->installEventFilter(this);

    auto iconLayout = new QVBoxLayout;
    iconLayout->setContentsMargins(0, 2, 0, 0);
    iconLayout->addWidget(m_pExpIcon);
    iconLayout->addStretch();

    QHBoxLayout *expLayout = new QHBoxLayout();
    expLayout->setContentsMargins(20, 0, 20, 0);
    expLayout->setSpacing(5);
    expLayout->addWidget(m_pExpCheckbox);
    expLayout->addLayout(iconLayout);
    expLayout->addStretch();
    expLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 10);
    layout->setSpacing(0);
    layout->addWidget(titleBar);
    layout->addSpacing(10);
    layout->addLayout(scrollLayout);
    layout->addSpacing(10);
    layout->addLayout(agrLayout);
    layout->addSpacing(10);
    layout->addLayout(expLayout);
    layout->addSpacing(10);

    this->setLayout(layout);
}

void UserAgreementDialog::initConnect()
{
    connect(m_pExpCheckbox, &WrapCheckBox::stateChanged, this, [](int state) {
        DbWrapper::localDbWrapper().updateUserExpState(state == Qt::Unchecked ? -1 : 1);
        ServerWrapper::instance()->updateUserExpState(state == Qt::Unchecked ? -1 : 1);
    });
}

DArrowRectangle *UserAgreementDialog::showArrowRectangle(DArrowRectangle::ArrowDirection direction)
{
    auto pExpTips = new DArrowRectangle(direction, DArrowRectangle::FloatWidget, this);
    pExpTips->setRadiusArrowStyleEnable(true);
    pExpTips->setRadius(16);
    QColor color = DGuiApplicationHelper::instance()->applicationPalette().color(QPalette::Base);
    color.setAlphaF(0.3);
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

bool UserAgreementDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pExpIcon) {
        if (event->type() == QEvent::Enter) {
            //获取当前鼠标位置
            QPoint curPos = QCursor::pos();
            //获取屏幕大小
            QRect desktopRect = QApplication::desktop()->rect();
            //根据提示框在屏幕的位置设置箭头方向
            QPoint p = mapToGlobal(m_pExpIcon->pos());
            if (curPos.x() + 300 > desktopRect.width()) {
                auto tips = showArrowRectangle(DArrowRectangle::ArrowRight);
                tips->show(p.x(), p.y() + 10);
            } else {
                auto tips = showArrowRectangle(DArrowRectangle::ArrowLeft);
                tips->show(p.x() + 20, p.y() + 10);
            }
        } else if (event->type() == QEvent::Leave) {
            auto tips = this->findChild<DArrowRectangle *>();
            if (tips) tips->deleteLater();
        } else if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
            return true;
        }
    }
    return DAbstractDialog::eventFilter(watched, event);
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
    }
    return content;
}
