#include "addskilldialog.h"
#include "iconcommandlinkbutton.h"
#include "gui/gutils.h"
#include "utils/util.h"
#include "utils/esystemcontext.h"
#include "themedlable.h"
#include "wordwizard/wordwizard.h"
#include "utils/dconfigmanager.h"
#include "private/echatwndmanager.h"
#include "tooltipwidget.h"

#include <DPaletteHelper>
#include <DGuiApplicationHelper>
#include <DTitlebar>
#include <DLabel>
#include <DFontSizeManager>
#include <DPlatformWindowHandle>
#include <DFrame>
#include <DSizeMode>

#include <QApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QBitmap>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QGraphicsDropShadowEffect>
#include <QToolTip>
#include <QGuiApplication>
#include <QScreen>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

AddSkillDialog::AddSkillDialog(DWidget *parent)
    : DAbstractDialog(parent)
{
    EWndManager()->registeWindow(this);
    initUI();
    initConnect();
    setModal(true);
    if (ESystemContext::isWayland()) {
        //wayland环境模态dialog需要手动置顶
        setWindowFlags(windowFlags() | Qt::Dialog |  Qt::WindowStaysOnTopHint);
    }
}

void AddSkillDialog::initUI()
{
    setFixedWidth(518);

    DTitlebar *titleBar = new DTitlebar(this);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);
    DFontSizeManager::instance()->bind(titleBar, DFontSizeManager::T5, QFont::DemiBold);
    titleBar->setTitle(tr("Add Skill"));

    DLabel *skillNameLabel = new DLabel(tr("Skill Name"));
    skillNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(skillNameLabel, DFontSizeManager::T6, QFont::Medium);

    m_pSkillNameLineEdit = new DLineEdit();
    m_pSkillNameLineEdit->setPlaceholderText(tr("Enter skill name"));
    m_pSkillNameLineEdit->setFixedHeight(36);
    m_pSkillNameLineEdit->lineEdit()->setMaxLength(21);
    m_pSkillNameLineEdit->lineEdit()->installEventFilter(this);
    m_pSkillNameLineEdit->lineEdit()->setProperty("_d_dtk_lineedit_opacity", false);
    m_pSkillNameLineEdit->setClearButtonEnabled(false);

    m_pCharCountLabel = new DLabel("0/20", m_pSkillNameLineEdit);
    m_pCharCountLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_pCharCountLabel, DFontSizeManager::T8, QFont::Normal);
    m_pCharCountLabel->setForegroundRole(DPalette::PlaceholderText);
    m_pCharCountLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_pCharCountLabel->show();

    DLabel *skillCommandLabel = new DLabel(tr("Skill Command"));
    skillCommandLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(skillCommandLabel, DFontSizeManager::T6, QFont::Medium);

    QHBoxLayout *descLayout = new QHBoxLayout();
    descLayout->setContentsMargins(0, 0, 0, 0);
    descLayout->setSpacing(0);

    m_pClickableLabel = new ClickableLabel(tr(" {selection} "));
    DFontSizeManager::instance()->bind(m_pClickableLabel, DFontSizeManager::T8, QFont::Normal);
    m_pClickableLabel->setForegroundRole(DPalette::TextTips);

    DLabel *descLabel = new DLabel(tr("represents the text selected by word selection."));
    descLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    descLabel->setForegroundRole(DPalette::TextTips);
    DFontSizeManager::instance()->bind(descLabel, DFontSizeManager::T8, QFont::Normal);

    m_pInstructionButton = new IconCommandLinkButton(tr("Instructions"), IconPosition::Right, this);
    m_pInstructionButton->setIcon(QIcon::fromTheme("uos-ai-assistant_tips"));
    DFontSizeManager::instance()->bind(m_pInstructionButton, DFontSizeManager::T8, QFont::Normal);

    descLayout->addWidget(m_pClickableLabel);
    descLayout->addWidget(descLabel);
    descLayout->addWidget(m_pInstructionButton);
    descLayout->addStretch();

    m_pSkillCommandTextEdit = new SkillCommandTextEdit();
    m_pSkillCommandTextEdit->setFixedHeight(120);
    m_pSkillCommandTextEdit->installEventFilter(this);

    QVBoxLayout *contentLayout = new QVBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(10);
    contentLayout->addWidget(skillNameLabel);
    contentLayout->addWidget(m_pSkillNameLineEdit);
    contentLayout->addSpacing(10);
    contentLayout->addWidget(skillCommandLabel);
    contentLayout->addLayout(descLayout);
    contentLayout->addWidget(m_pSkillCommandTextEdit);

    QWidget *contentWidget = new QWidget(this);
    contentWidget->setLayout(contentLayout);

    m_pCancelButton = new DPushButton(tr("Cancel"));
    m_pCancelButton->setFixedWidth(222);
    m_pSubmitButton = new DSuggestButton(tr("Save"));
    m_pSubmitButton->setFixedWidth(222);
    m_pSubmitButton->setEnabled(false);

    DVerticalLine *verticalLine = new DVerticalLine(this);
    verticalLine->setObjectName("VLine");
    verticalLine->setFixedHeight(DSizeModeHelper::element(20, 30));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(5);
    buttonLayout->addWidget(m_pCancelButton);
    buttonLayout->addWidget(verticalLine);
    buttonLayout->addWidget(m_pSubmitButton);
    buttonLayout->addStretch();

    QWidget *buttonWidget = new QWidget(this);
    buttonWidget->setLayout(buttonLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(titleBar);

    QWidget *contentContainer = new QWidget(this);
    QVBoxLayout *containerLayout = new QVBoxLayout(contentContainer);
    containerLayout->setContentsMargins(30, 0, 30, 10);
    containerLayout->setSpacing(10);
    containerLayout->addWidget(contentWidget);
    containerLayout->addStretch();
    containerLayout->addSpacing(15);
    containerLayout->addWidget(buttonWidget);
    
    mainLayout->addWidget(contentContainer);

    setLayout(mainLayout);
    onCompactModeChanged();
    QTimer::singleShot(0, this, &AddSkillDialog::updateCharCountLabelPosition);
}

void AddSkillDialog::initConnect()
{
    connect(m_pCancelButton, &DPushButton::clicked, this, &AddSkillDialog::onCancelButtonClicked);
    connect(m_pSubmitButton, &DPushButton::clicked, this, &AddSkillDialog::onSubmitButtonClicked);
    connect(m_pSkillNameLineEdit, &DLineEdit::textChanged, this, &AddSkillDialog::onSkillNameChanged);
    connect(m_pSkillNameLineEdit, &DLineEdit::alertChanged, this, &AddSkillDialog::onSkillNameAlertChanged);
    connect(m_pSkillCommandTextEdit, &DTextEdit::textChanged, this, &AddSkillDialog::onSkillCommandChanged);
    connect(m_pClickableLabel, &ClickableLabel::clicked, this, &AddSkillDialog::onInsertSelectedContentTag);
    connect(m_pInstructionButton, &IconCommandLinkButton::clicked, this, &AddSkillDialog::showInstructionDialog);

    connect(m_pSkillCommandTextEdit, &SkillCommandTextEdit::tagDeleted, this, &AddSkillDialog::onTagDeleted);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged,
            this, &AddSkillDialog::onCompactModeChanged);
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)),
            this, SLOT(onUpdateSystemFont(const QFont &)));
}

bool AddSkillDialog::isSkillNameValid() const
{
    QString name = m_pSkillNameLineEdit->text().trimmed();
    return !name.isEmpty() && name.length() <= 20;
}

bool AddSkillDialog::isSkillCommandValid() const
{
    QString name = m_pSkillCommandTextEdit->toPlainText().trimmed();
    return !name.isEmpty() && name.length() <= 500;
}

QString AddSkillDialog::getSkillName() const
{
    return m_skillName;
}

QString AddSkillDialog::getSkillCommand() const
{
    return m_skillCommand;
}

void AddSkillDialog::resetDialog()
{
    m_pSkillNameLineEdit->clear();
    m_pSkillCommandTextEdit->resetToDefault();
    m_skillName.clear();
    m_skillCommand.clear();
    m_originalSkillName.clear();

    m_pCharCountLabel->setText("0/20");
    m_pCharCountLabel->setForegroundRole(DPalette::PlaceholderText);

    onUpdateSubmitButtonStatus();
}

void AddSkillDialog::setSkillData(const QString &name, const QString &command)
{
    m_originalSkillName = name;
    m_pSkillNameLineEdit->setText(name);
    m_pSkillCommandTextEdit->setTextFromSaved(command);

    int currentLength = name.length();
    m_pCharCountLabel->setText(QString("%1/20").arg(currentLength));

    m_pSkillNameLineEdit->setAlert(false);
    m_pSkillNameLineEdit->hideAlertMessage();

    onUpdateSubmitButtonStatus();
}

void AddSkillDialog::onSubmitButtonClicked()
{
    m_skillName = m_pSkillNameLineEdit->text().trimmed();
    m_skillCommand = m_pSkillCommandTextEdit->getTextForSave().trimmed();

    if (m_skillName.isEmpty()) {
        qCWarning(logAIGUI) << "Cannot submit: skill name is empty";
        m_pSkillNameLineEdit->setAlert(true);
        m_pSkillNameLineEdit->showAlertMessage(tr("Skill name cannot be empty"), -1);
        return;
    }

    if (m_skillCommand.isEmpty()) {
        qCWarning(logAIGUI) << "Cannot submit: skill command is empty";
        return;
    }

    if (isNameDuplicate()) {
        qCWarning(logAIGUI) << "Cannot submit: skill name already exists:" << m_skillName;
        return;
    }

    accept();
}

void AddSkillDialog::onCancelButtonClicked()
{
    reject();
}

void AddSkillDialog::onSkillNameChanged(const QString &text)
{
    QString noEnterText = text;
    if (noEnterText.contains("\n")) {
        noEnterText.replace("\n", " ");
        int cursorPos = m_pSkillNameLineEdit->lineEdit()->cursorPosition();
        m_pSkillNameLineEdit->setText(noEnterText);
        m_pSkillNameLineEdit->lineEdit()->setCursorPosition(cursorPos);
        qCWarning(logAIGUI) << "Line breaks were removed from skill name input";
    }

    int currentLength = noEnterText.length();
    m_pCharCountLabel->setText(QString("%1/20").arg(currentLength));
    m_pSkillNameLineEdit->setAlert(false);
    m_pSkillNameLineEdit->hideAlertMessage();

    if (noEnterText.length() > 20) {
        qCWarning(logAIGUI) << "Skill name exceeded 20 character limit, truncating";
        m_pSkillNameLineEdit->blockSignals(true);
        m_pSkillNameLineEdit->setText(noEnterText.left(20));
        m_pSkillNameLineEdit->blockSignals(false);
        m_pCharCountLabel->setText("20/20");
        m_pSkillNameLineEdit->setAlert(true);
        m_pSkillNameLineEdit->showAlertMessage(tr("Exceeded character limit"), -1);
    }

    onUpdateSubmitButtonStatus();
}

void AddSkillDialog::onSkillCommandChanged()
{
    onUpdateSubmitButtonStatus();
}

void AddSkillDialog::onUpdateSubmitButtonStatus()
{
    bool disable = !isSkillNameValid() || !isSkillCommandValid();
    m_pSubmitButton->setDisabled(disable);
}

void AddSkillDialog::onCompactModeChanged()
{
    int height = GUtils::isCompactMode() ? 24 : 36;

    if (m_pSkillNameLineEdit) {
        m_pSkillNameLineEdit->setFixedHeight(height);
    }
    if (m_pCancelButton) {
        m_pCancelButton->setFixedHeight(height);
    }
    if (m_pSubmitButton) {
        m_pSubmitButton->setFixedHeight(height);
    }

    adjustSize();

    updateCharCountLabelPosition();
}

void AddSkillDialog::onUpdateSystemFont(const QFont &)
{
    adjustSize();
    updateCharCountLabelPosition();
    updateTooltipPosition();
}

void AddSkillDialog::updateCharCountLabelPosition()
{
    if (m_pCharCountLabel && m_pSkillNameLineEdit) {
        QString currentText = m_pCharCountLabel->text();
        QFontMetrics fm(m_pCharCountLabel->font());
        int textWidth = fm.horizontalAdvance(currentText);
        int textHeight = fm.height();

        int padding = 8;
        int labelWidth = textWidth + padding;
        int labelHeight = qMax(textHeight, 20); // 最小高度20

        m_pCharCountLabel->setFixedSize(labelWidth, labelHeight);

        int x = m_pSkillNameLineEdit->width() - labelWidth - 10;
        int y = (m_pSkillNameLineEdit->height() - labelHeight) / 2;
        m_pCharCountLabel->move(x, y);
        m_pCharCountLabel->raise();

        // 关键：设置右侧内边距，避免文本遮挡计数
        m_pSkillNameLineEdit->lineEdit()->setTextMargins(0, 0, labelWidth + 10, 0);
    }
}

bool AddSkillDialog::event(QEvent *event)
{
    if (event->type() == QEvent::Close || event->type() == QEvent::Hide) {
        hideTooltip(false);
    }

    if (event->type() == QEvent::KeyPress) {
        if (!m_pSubmitButton->isEnabled()) {
            event->ignore();
            return true;
        }
        auto keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
            emit m_pSubmitButton->clicked();
            event->ignore();
            return true;
        }
    }

    return DAbstractDialog::event(event);
}

bool AddSkillDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_pSkillNameLineEdit->lineEdit() && event->type() == QEvent::FocusOut) {
        isNameDuplicate();
    }
    
    return DAbstractDialog::eventFilter(watched, event);
}

bool AddSkillDialog::isNameDuplicate() const
{
    QString currentName = m_pSkillNameLineEdit->text().trimmed();
    
    // 如果是编辑模式且名称没有变化，不算重复
    if (!m_originalSkillName.isEmpty() && currentName == m_originalSkillName) {
        return false;
    }
    
    for (const auto &function : WordWizard::kCustomFunctionList) {
        QString existingName = function.isCustom ? function.name : WordWizard::getDefaultSkillName(function.defaultFunctionType);
        if (existingName == currentName) {
            qCWarning(logAIGUI) << "Skill name already exists:" << currentName;
            m_pSkillNameLineEdit->setAlert(true);
            m_pSkillNameLineEdit->showAlertMessage(tr("The skill name already exists."), -1);
            return true;
        }
    }
    return false;
}

void AddSkillDialog::onInsertSelectedContentTag()
{
    if (m_pSkillCommandTextEdit) {
        m_pSkillCommandTextEdit->insertSelectedContentTag();
        m_pSkillCommandTextEdit->setFocus();
    }

    if (m_pTooltipWidget && m_pTooltipWidget->isVisible()) {
        hideTooltip(true);
    }
}

void AddSkillDialog::onSkillNameAlertChanged(bool alert)
{
    if (alert)
        m_pSubmitButton->setDisabled(true);
}

void AddSkillDialog::showInstructionDialog()
{
    DAbstractDialog *instructionDialog = new DAbstractDialog(this);
    instructionDialog->setModal(true);
    instructionDialog->setFixedWidth(518);

    DTitlebar *titleBar = new DTitlebar(instructionDialog);
    titleBar->setMenuVisible(false);
    titleBar->setBackgroundTransparent(true);
    DFontSizeManager::instance()->bind(titleBar, DFontSizeManager::T6, QFont::Medium);
    titleBar->setTitle(tr("Command Instructions"));

    DLabel *imageLabel = new DLabel(instructionDialog);
    QString iconName = QString(":/icons/deepin/builtin/%1/icons/");
    if (Util::checkLanguage())
        iconName += "display.svg";
    else
        iconName += "display_en.svg";

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
        iconName = iconName.arg("light");
    else
        iconName = iconName.arg("dark");
    const QSize imageSize(458, 116);
    imageLabel->setPixmap(Util::loadSvgPixmap(iconName, imageSize));
    imageLabel->setFixedSize(imageSize);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setAutoFillBackground(true);

    // 创建圆角遮罩
    QBitmap mask(imageSize);
    mask.fill(Qt::color0);
    QPainter maskPainter(&mask);
    maskPainter.setRenderHint(QPainter::Antialiasing);
    maskPainter.setBrush(Qt::color1);
    maskPainter.setPen(Qt::NoPen);
    maskPainter.drawRoundedRect(0, 0, imageSize.width(), imageSize.height(), 8, 8);
    maskPainter.end();
    imageLabel->setMask(mask);

    DPalette imagePalette = imageLabel->palette();
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        imagePalette.setColor(DPalette::Window, QColor(0, 0, 0, 8)); // rgba(0, 0, 0, 0.03) = alpha 8/255
    } else {
        imagePalette.setColor(DPalette::Window, QColor(255, 255, 255, 13)); // rgba(255, 255, 255, 0.05) 深色背景
    }
    imageLabel->setPalette(imagePalette);

    ThemedLable *descriptionLabel = new ThemedLable(tr("If you want to translate the selected text \"Hello\", as shown above, you can enter the command: Translate {selection} into English."));
    descriptionLabel->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.7);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    descriptionLabel->setFixedWidth(458);
    DFontSizeManager::instance()->bind(descriptionLabel, DFontSizeManager::T6, QFont::Normal);

    ThemedLable *advancedTitleLabel = new ThemedLable(tr("Advanced Tips"));
    advancedTitleLabel->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.85);
    advancedTitleLabel->setAlignment(Qt::AlignLeft);
    advancedTitleLabel->setFixedWidth(458);
    DFontSizeManager::instance()->bind(advancedTitleLabel, DFontSizeManager::T6, QFont::Medium);

    ThemedLable *advancedLabel = new ThemedLable(tr("The more specific the command, the more accurate the generated content. For example:\n• Generate a PPT outline for {selection} with 3 chapters and 10 key points.\n• Polish {selection} into a recruitment copy within 100 words, highlighting teamwork."));
    advancedLabel->setPaletteColor(QPalette::Text, QPalette::BrightText, 0.7);
    advancedLabel->setWordWrap(true);
    advancedLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    advancedLabel->setFixedWidth(458);
    DFontSizeManager::instance()->bind(advancedLabel, DFontSizeManager::T6, QFont::Normal);

    QWidget *contentContainer = new QWidget(this);
    QVBoxLayout *containerLayout = new QVBoxLayout(contentContainer);
    containerLayout->setContentsMargins(30, 0, 30, 0);
    containerLayout->setSpacing(0);
    containerLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
    containerLayout->addSpacing(10);
    containerLayout->addWidget(descriptionLabel);
    containerLayout->addSpacing(24);
    containerLayout->addWidget(advancedTitleLabel);
    containerLayout->addSpacing(6);
    containerLayout->addWidget(advancedLabel);
    containerLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(instructionDialog);
    mainLayout->setContentsMargins(0, 0, 0, 50);
    mainLayout->setSpacing(15);
    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(contentContainer);

    instructionDialog->setLayout(mainLayout);
    instructionDialog->exec();
    
    instructionDialog->deleteLater();
}

void AddSkillDialog::onTagDeleted()
{
    bool isFirstShow = DConfigManager::instance()->value(WORDWIZARD_GROUP, WORDWIZARD_LABELTOOLTIP, true).toBool();
    if (isFirstShow) {
        showNativeTooltip();
    }
}

void AddSkillDialog::showNativeTooltip()
{
    if (!m_pTooltipWidget) {
        m_pTooltipWidget = new TooltipWidget(this);
        m_pTooltipWidget->setContentsMargins(10, 10, 10, 10); // 给阴影留空间
        m_pTooltipTextLabel = new DLabel(m_pTooltipWidget);
        m_pTooltipTextLabel->setWordWrap(false);
        DFontSizeManager::instance()->bind(m_pTooltipTextLabel, DFontSizeManager::T8, QFont::Normal);
        m_pTooltipTextLabel->setTextFormat(Qt::RichText);
        m_pTooltipTextLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        m_pTooltipTextLabel->setOpenExternalLinks(false);

        connect(m_pTooltipTextLabel, &DLabel::linkActivated, this, [this](const QString &link) {
            if (link == "ok") {
                hideTooltip(true);
            }
        });

        QHBoxLayout *layout = new QHBoxLayout(m_pTooltipWidget);
        layout->setContentsMargins(4, 2, 4, 2);
        layout->setSpacing(0);
        layout->addWidget(m_pTooltipTextLabel);
    }

    if (m_pClickableLabel && m_pTooltipWidget) {
        QColor linkColor = DGuiApplicationHelper::instance()->applicationPalette().color(DPalette::Highlight);
        QColor textColor = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType 
                          ? QColor(0, 0, 0, 255)
                          : QColor(255, 255, 255, 255);
        
        QString richText = QString("<span style=\"color: %1;\">%2</span><a href=\"ok\" style=\"color: %3; text-decoration: none;\">%4</a>")
                          .arg(textColor.name())
                          .arg(tr("Clicking the label can still insert it into the input field."))
                          .arg(linkColor.name())
                          .arg(tr("OK"));
        
        m_pTooltipTextLabel->setText(richText);
        m_pTooltipWidget->adjustSize();
        m_pTooltipWidget->setFixedSize(m_pTooltipWidget->size());

        m_pTooltipWidget->show();
        m_pTooltipWidget->raise();
        updateTooltipPosition();
    }
}

void AddSkillDialog::hideTooltip(bool isDisabled)
{
    if (m_pTooltipWidget) {
        m_pTooltipWidget->hide();
    }

    if (isDisabled) {
        DConfigManager::instance()->setValue(WORDWIZARD_GROUP, WORDWIZARD_LABELTOOLTIP, false);
    }
}

void AddSkillDialog::updateTooltipPosition()
{
    if (m_pTooltipWidget && m_pTooltipWidget->isVisible() && m_pClickableLabel) {
        m_pTooltipWidget->adjustSize();

        QPoint labelGlobalPos = m_pClickableLabel->mapToGlobal(QPoint(0, 0));
        int tooltipX = labelGlobalPos.x() + m_pClickableLabel->width() + 3 - 10;
        int tooltipY = labelGlobalPos.y() + (m_pClickableLabel->height() - m_pTooltipWidget->height()) / 2;

        // 获取屏幕几何信息，确保tooltip在屏幕内
        QScreen *screen = QGuiApplication::screenAt(labelGlobalPos);
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
        QRect screenGeometry = screen->availableGeometry();

        int maxX = screenGeometry.right() - m_pTooltipWidget->width();
        int maxY = screenGeometry.bottom() - m_pTooltipWidget->height();
        tooltipX = qMax(screenGeometry.left(), qMin(tooltipX, maxX));
        tooltipY = qMax(screenGeometry.top(), qMin(tooltipY, maxY));

        // 如果右侧空间不够，显示在左侧
        if (tooltipX + m_pTooltipWidget->width() > screenGeometry.right()) {
            tooltipX = labelGlobalPos.x() - m_pTooltipWidget->width();
            tooltipX = qMax(screenGeometry.left(), tooltipX);
        }

        m_pTooltipWidget->move(tooltipX, tooltipY);
    }
}

void AddSkillDialog::moveEvent(QMoveEvent *event)
{
    DAbstractDialog::moveEvent(event);
    updateTooltipPosition();
}
