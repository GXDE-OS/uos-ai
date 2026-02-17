#include "skillcommandtextedit.h"

#include <DPaletteHelper>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DAlertControl>

#include <QPainter>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextLayout>
#include <QTextLine>
#include <QTextBlock>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QAbstractTextDocumentLayout>
#include <QRegularExpression>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QPlainTextDocumentLayout>
#include <QTimer>
#include <QLoggingCategory>

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static inline QString getTagText() {
    return QObject::tr("{selection}");
}

SkillCommandTextEdit::SkillCommandTextEdit(QWidget *parent)
    : DTextEdit(parent)
    , m_hasUserInput(false)
    , m_placeholderVisible(true)
    , m_isInitialized(false)
    , m_userHasEdited(false)
    , m_internalOperation(false)
    , m_pCharCountLabel(nullptr)
    , m_alertControl(nullptr)
    , m_selectionTagObject(new SelectionTagObject(this))
{
    initUI();
    initConnect();
    setupSelectionTagHandler();
    updateViewportMargins();
    setupDefaultContent();
}

void SkillCommandTextEdit::initUI()
{
    m_plainTextPart = tr("Take ");
    m_placeholderPart = tr(" translate into English");

    m_pCharCountLabel = new DLabel("0/500", this);
    m_pCharCountLabel->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_pCharCountLabel, DFontSizeManager::T8, QFont::Normal);
    m_pCharCountLabel->setForegroundRole(DPalette::PlaceholderText);
    m_pCharCountLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_pCharCountLabel->show();

    m_alertControl = new DAlertControl(this, this);
}

void SkillCommandTextEdit::initConnect()
{
    connect(this, &DTextEdit::textChanged, this, &SkillCommandTextEdit::onTextChanged);
    connect(this, &DTextEdit::cursorPositionChanged, this, &SkillCommandTextEdit::onCursorPositionChanged);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &SkillCommandTextEdit::onThemeChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::paletteTypeChanged,
            this, &SkillCommandTextEdit::onThemeChanged);

    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)),
            this, SLOT(onFontChanged(const QFont &)));
}

void SkillCommandTextEdit::setupSelectionTagHandler()
{
    document()->documentLayout()->registerHandler(SelectionTagType, m_selectionTagObject);
}

void SkillCommandTextEdit::setupDefaultContent()
{
    blockSignals(true);

    setPlainText(m_plainTextPart);
    QTextCursor cursor = textCursor();
    cursor.setPosition(m_plainTextPart.length());
    SelectionTagObject::insertSelectionTag(cursor);

    QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
    if (tagPos.first != -1) {
        cursor.setPosition(tagPos.second);
        setTextCursor(cursor);
    }

    m_isInitialized = true;
    m_hasUserInput = false;
    m_placeholderVisible = true;

    blockSignals(false);

    updateCharCountLabel();
    updateCharCountLabelPosition();
    update();
}

void SkillCommandTextEdit::resetToDefault()
{
    m_isInitialized = false;
    m_hasUserInput = false;
    m_placeholderVisible = true;
    m_userContent.clear();
    m_userHasEdited = false;
    m_internalOperation = false;

    SelectionTagObject::removeAllSelectionTags(document());

    if (m_alertControl) {
        setAlert(false);
        hideAlertMessage();
    }

    setupDefaultContent();
}

QString SkillCommandTextEdit::getTextForSave() const
{
    if (m_placeholderVisible && !m_userHasEdited) {
        return m_plainTextPart + "${SELECTION}" + m_placeholderPart;
    }

    return SelectionTagObject::extractSaveFormatFromDocument(document());
}

void SkillCommandTextEdit::setTextFromSaved(const QString &text)
{
    blockSignals(true);

    clear();
    int dollarTagIndex = text.indexOf("${SELECTION}");
    if (dollarTagIndex != -1) {
        QString beforeTag = text.left(dollarTagIndex);
        QString afterTag = text.mid(dollarTagIndex + 12); // "${SELECTION}"长度为12

        QTextCursor cursor = textCursor();

        QTextCharFormat defaultFormat;
        defaultFormat.setObjectType(QTextFormat::NoObject);

        if (!beforeTag.isEmpty()) {
            cursor.setCharFormat(defaultFormat);
            cursor.insertText(beforeTag);
        }
        SelectionTagObject::insertSelectionTag(cursor);
        if (!afterTag.isEmpty()) {
            cursor.setCharFormat(defaultFormat);
            cursor.insertText(afterTag);
        }
        QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
        if (tagPos.first != -1) {
            cursor.setPosition(tagPos.second);
            setTextCursor(cursor);
        }
    } else {
        QString processedText = SelectionTagObject::convertFromSaveFormat(text);
        setPlainText(processedText);
        QTextCursor cursor(document());
        cursor.select(QTextCursor::Document);
        QTextCharFormat defaultFormat;
        defaultFormat.setObjectType(QTextFormat::NoObject);
        cursor.setCharFormat(defaultFormat);
    }

    QString currentText = toPlainText();
    if (currentText.length() > 500) {
        qCWarning(logAIGUI) << "Loaded text exceeds character limit";
        blockSignals(false);
        setAlert(true);
        showAlertMessage(tr("Exceeded character limit"), -1);
        return;
    }

    blockSignals(false);

    m_hasUserInput = !currentText.isEmpty();
    m_placeholderVisible = false;
    m_userHasEdited = !currentText.isEmpty();
    m_isInitialized = true;

    if (isAlert()) {
        setAlert(false);
        hideAlertMessage();
    }

    updateCharCountLabel();
    updateCharCountLabelPosition();
    update();
}

void SkillCommandTextEdit::insertSelectedContentTag()
{
    bool hasTag = SelectionTagObject::hasSelectionTag(document());

    if (hasTag) {
        SelectionTagObject::removeAllSelectionTags(document());
    }

    QString currentText = toPlainText();
    if (currentText.length() + 1 > 500) {
        qCWarning(logAIGUI) << "Cannot insert tag: would exceed character limit";
        setAlert(true);
        showAlertMessage(tr("Exceeded character limit"), -1);
        return;
    }

    m_internalOperation = true;

    QTextCursor cursor = textCursor();

    if (!hasFocus()) {
        cursor.movePosition(QTextCursor::End);
    }

    SelectionTagObject::insertSelectionTag(cursor);

    QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
    if (tagPos.first != -1) {
        cursor.setPosition(tagPos.second);
        setTextCursor(cursor);
    }

    m_hasUserInput = true;
    m_placeholderVisible = false;
    m_userHasEdited = true;

    m_internalOperation = false;

    updateCharCountLabel();
    updateCharCountLabelPosition();
    update();
}

void SkillCommandTextEdit::paintEvent(QPaintEvent *event)
{
    DTextEdit::paintEvent(event);

    if (m_placeholderVisible && !m_userHasEdited && !m_placeholderPart.isEmpty()) {
        // 初始化构造明文+暗文
        QString currentText = toPlainText();
        bool hasOnlyDefaultContent = false;

        QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
        if (tagPos.first != -1) {
            QString expectedContent = m_plainTextPart + QChar::ObjectReplacementCharacter;

            hasOnlyDefaultContent = (currentText == expectedContent);
        }

        if (hasOnlyDefaultContent ||
            (currentText.length() == m_plainTextPart.length() + 1 &&
             currentText.startsWith(m_plainTextPart) &&
             currentText.endsWith(QChar::ObjectReplacementCharacter))) {

            QPainter painter(viewport());

            DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
            QColor placeholderColor = palette.color(DPalette::PlaceholderText);

            painter.setPen(placeholderColor);
            painter.setFont(font());

            QTextDocument *doc = document();
            qreal docMargin = doc->documentMargin();

            doc->documentLayout()->update();

            QTextBlock firstBlock = doc->firstBlock();
            QTextLayout *layout = firstBlock.layout();

            if (layout && layout->lineCount() > 0) {
                QTextLine firstLine = layout->lineAt(0);

                qreal actualWidth = firstLine.cursorToX(tagPos.second);

                int x = static_cast<int>(docMargin + actualWidth);
                int y = static_cast<int>(docMargin + firstLine.ascent());

                painter.drawText(x, y, m_placeholderPart);
            } else {
                QFontMetrics fm(font());
                int plainTextWidth = fm.horizontalAdvance(m_plainTextPart);
                QString tagText = SelectionTagObject::getDefaultTagText();
                int tagWidth = fm.horizontalAdvance(tagText) + 8;

                int x = static_cast<int>(docMargin + plainTextWidth + tagWidth);
                int y = static_cast<int>(docMargin + fm.ascent());

                painter.drawText(x, y, m_placeholderPart);
            }
        }
    }
}

void SkillCommandTextEdit::resizeEvent(QResizeEvent *event)
{
    DTextEdit::resizeEvent(event);
    updateCharCountLabelPosition();
}

void SkillCommandTextEdit::keyPressEvent(QKeyEvent *event)
{
    // 首先检查是否有选中的文本
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        int selectionStart = cursor.selectionStart();
        int selectionEnd = cursor.selectionEnd();

        // 如果是删除或退格键，并且选中区域包含标签，则发出标签删除信号
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            bool tagAffected = false;

            // 检查选中区域是否包含标签
            QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
            if (tagPos.first != -1) {
                if (!(tagPos.second <= selectionStart || tagPos.first >= selectionEnd)) {
                    tagAffected = true;
                }
            }

            DTextEdit::keyPressEvent(event);

            if (tagAffected) {
                emit tagDeleted();
            }

            m_placeholderVisible = false;
            m_userHasEdited = true;
            update();
            return;
        } else {
            // 非删除键，允许正常处理选中文本的替换
            QString insertText = event->text();
            if (!insertText.isEmpty() && insertText.at(0).isPrint()) {
                bool tagAffected = false;

                // 检查选中区域是否包含标签
                QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
                if (tagPos.first != -1) {
                    if (!(tagPos.second <= selectionStart || tagPos.first >= selectionEnd)) {
                        tagAffected = true;
                    }
                }

                DTextEdit::keyPressEvent(event);

                m_placeholderVisible = false;
                m_userHasEdited = true;
                update();
                return;
            }
        }
    }

    // 检查是否在选择标签内
    int tagStart, tagEnd;
    int cursorPos = textCursor().position();

    // 只有当光标真正在标签内部时才进行特殊处理
    if (SelectionTagObject::isPositionInSelectionTag(document(), cursorPos, &tagStart, &tagEnd) &&
        cursorPos > tagStart && cursorPos < tagEnd) {
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            // 删除整个标签
            QTextCursor cursor = textCursor();
            cursor.setPosition(tagStart);
            cursor.setPosition(tagEnd, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();

            m_placeholderVisible = false;
            m_userHasEdited = true;

            emit tagDeleted();

            onTextChanged();

            update();
            return;
        } else if (event->key() == Qt::Key_Left) {
            m_internalOperation = true;
            QTextCursor cursor = textCursor();
            cursor.setPosition(tagStart);
            setTextCursor(cursor);
            m_internalOperation = false;
            return;
        } else if (event->key() == Qt::Key_Right) {
            m_internalOperation = true;
            QTextCursor cursor = textCursor();
            cursor.setPosition(tagEnd);
            setTextCursor(cursor);
            m_internalOperation = false;
            return;
        } else if (event->text().length() > 0 && event->text().at(0).isPrint()) {
            return;
        }
    }

    // 如果是第一次输入（除了标签），隐藏暗文
    if (m_placeholderVisible && event->key() != Qt::Key_Tab &&
        event->key() != Qt::Key_Shift && event->key() != Qt::Key_Control &&
        event->key() != Qt::Key_Alt && event->key() != Qt::Key_Meta &&
        event->key() != Qt::Key_Left && event->key() != Qt::Key_Right &&
        event->key() != Qt::Key_Up && event->key() != Qt::Key_Down)
    {
        m_placeholderVisible = false;
        m_userHasEdited = true;
        update();
    }

    // 处理光标在标签边界时的方向键操作
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        int tagStart, tagEnd;

        if (event->key() == Qt::Key_Left) {
            if (SelectionTagObject::isPositionInSelectionTag(document(), cursorPos - 1, &tagStart, &tagEnd) &&
                cursorPos == tagEnd) {
                m_internalOperation = true;
                QTextCursor cursor = textCursor();
                cursor.setPosition(tagStart);
                setTextCursor(cursor);
                m_internalOperation = false;
                return;
            }
        } else if (event->key() == Qt::Key_Right) {
            if (SelectionTagObject::isPositionInSelectionTag(document(), cursorPos, &tagStart, &tagEnd) &&
                cursorPos == tagStart) {
                m_internalOperation = true;
                QTextCursor cursor = textCursor();
                cursor.setPosition(tagEnd);
                setTextCursor(cursor);
                m_internalOperation = false;
                return;
            }
        }
    }

    int oldCursorPos = textCursor().position();
    QString oldText = toPlainText();

    // 检查输入后是否会超出字符限制
    QString insertText = event->text();
    if (!insertText.isEmpty() && insertText.at(0).isPrint()) {
        if (oldText.length() >= 500) {
            setAlert(true);
            showAlertMessage(tr("Exceeded character limit"), -1);
            return;
        }
    }

    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QTextCursor cursor = textCursor();
        QString currentText = toPlainText();
        bool tagAffected = false;

        // 检查将要删除的字符是否会影响标签
        if (event->key() == Qt::Key_Backspace && cursor.position() > 0) {
            int posToCheck = cursor.position() - 1;
            int tagStart, tagEnd;
            if (SelectionTagObject::isPositionInSelectionTag(document(), posToCheck, &tagStart, &tagEnd) 
                    && cursor.position() == tagEnd && posToCheck == tagStart) {
                tagAffected = true;
            }

            cursor.setPosition(cursor.position() - 1);
            cursor.setPosition(cursor.position() + 1, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        } else if (event->key() == Qt::Key_Delete && cursor.position() < currentText.length()) {
            int posToCheck = cursor.position() + 1;
            int tagStart, tagEnd;
            if (SelectionTagObject::isPositionInSelectionTag(document(), posToCheck, &tagStart, &tagEnd)
                    && posToCheck == tagEnd && cursor.position() == tagStart) {
                tagAffected = true;
            }

            cursor.setPosition(cursor.position());
            cursor.setPosition(cursor.position() + 1, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }

        // 如果标签被影响，发出信号
        if (tagAffected) {
            emit tagDeleted();
        }

        m_placeholderVisible = false;
        m_userHasEdited = true;
        onTextChanged();
    } else {
        DTextEdit::keyPressEvent(event);

        QString newText = toPlainText();
        if (newText != oldText) {
            m_placeholderVisible = false;
            m_userHasEdited = true;
            onTextChanged();
        }
    }
}

void SkillCommandTextEdit::mousePressEvent(QMouseEvent *event)
{
    DTextEdit::mousePressEvent(event);

    // 处理鼠标点击在标签内
    QTextCursor cursor = cursorForPosition(event->pos());
    int position = cursor.position();
    int tagStart, tagEnd;

    if (SelectionTagObject::isPositionInSelectionTag(document(), position, &tagStart, &tagEnd) &&
        position > tagStart && position < tagEnd) {
        QTextCursor newCursor = textCursor();
        int tagMiddle = (tagStart + tagEnd) / 2;
        if (position <= tagMiddle) {
            newCursor.setPosition(tagStart);
        } else {
            newCursor.setPosition(tagEnd);
        }

        setTextCursor(newCursor);
    }
}

void SkillCommandTextEdit::mouseMoveEvent(QMouseEvent *event)
{
    // 处理文本拖拽
    if (event->buttons() & Qt::LeftButton) {
        QTextCursor cursor = cursorForPosition(event->pos());
        int position = cursor.position();
        int tagStart, tagEnd;

        if (SelectionTagObject::isPositionInSelectionTag(document(), position, &tagStart, &tagEnd)) {
            QTextCursor currentCursor = textCursor();
            int anchorPos = currentCursor.anchor();

            if (anchorPos <= tagStart) {
                currentCursor.setPosition(anchorPos);
                currentCursor.setPosition(tagStart, QTextCursor::KeepAnchor);
            } else if (anchorPos >= tagEnd) {
                currentCursor.setPosition(anchorPos);
                currentCursor.setPosition(tagEnd, QTextCursor::KeepAnchor);
            } else {
                currentCursor.setPosition(tagStart);
            }

            setTextCursor(currentCursor);
            return;
        }
    }

    DTextEdit::mouseMoveEvent(event);
}

void SkillCommandTextEdit::onTextChanged()
{
    if (!m_isInitialized || m_internalOperation) return;

    QString currentText = toPlainText();

    // 检查字符数限制（500个字符）
    if (currentText.length() > 500) {
        qCWarning(logAIGUI) << "Text length exceeded 500 characters, truncating";
        m_internalOperation = true;

        int cursorPos = textCursor().position();
        QString truncatedText = currentText.left(500);

        blockSignals(true);
        setPlainText(truncatedText);
        blockSignals(false);

        QTextCursor cursor = textCursor();
        int newPos = qMin(cursorPos, truncatedText.length());
        cursor.setPosition(newPos);
        setTextCursor(cursor);

        // 检查截断后标签是否还存在
        bool hadTag = SelectionTagObject::hasSelectionTag(document());
        if (hadTag) {
            // 如果截断前有标签但截断后没有，说明标签被删除了
            QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
            if (tagPos.first == -1) {
                qCWarning(logAIGUI) << "Tag was removed during text truncation";
            }
        }

        m_internalOperation = false;
        currentText = truncatedText;

        setAlert(true);
        showAlertMessage(tr("Exceeded character limit"), -1);
    } else {
        if (isAlert()) {
            setAlert(false);
            hideAlertMessage();
        }
    }

    // 检查是否处于默认状态
    bool isDefaultState = false;
    QPair<int, int> tagPos = SelectionTagObject::findSelectionTagPosition(document());
    if (tagPos.first != -1) {
        QString expectedContent = m_plainTextPart + QChar::ObjectReplacementCharacter;
        isDefaultState = (currentText == expectedContent);
    }

    bool isDefaultStateFixed = isDefaultState ||
        (tagPos.first != -1 &&
         currentText.length() == m_plainTextPart.length() + 1 &&
         currentText.startsWith(m_plainTextPart) &&
         currentText.endsWith(QChar::ObjectReplacementCharacter));

    if (!isDefaultStateFixed) {
        m_hasUserInput = true;
        m_userHasEdited = true;
        m_placeholderVisible = false;
    } else {
        m_hasUserInput = false;
        if (m_userHasEdited) {
            m_placeholderVisible = false;
        } else {
            m_placeholderVisible = true;
        }
    }

    updateCharCountLabel();
    updateCharCountLabelPosition();
    update();
}

void SkillCommandTextEdit::onCursorPositionChanged()
{
    if (m_internalOperation) return;

    // 如果用户刚刚编辑过内容，暂时延迟处理光标位置调整
    // 避免在文本变化和格式化过程中错误地移动光标
    if (m_userHasEdited) {
        QTimer::singleShot(0, this, [this]() {
            this->checkCursorPosition();
        });
        return;
    }

    checkCursorPosition();
}

void SkillCommandTextEdit::checkCursorPosition()
{
    if (m_internalOperation) return;

    int cursorPos = textCursor().position();
    int tagStart, tagEnd;

    if (SelectionTagObject::isPositionInSelectionTag(document(), cursorPos, &tagStart, &tagEnd) &&
        cursorPos > tagStart && cursorPos < tagEnd) {
        QTextCursor cursor = textCursor();

        if (cursorPos - tagStart < tagEnd - cursorPos) {
            cursor.setPosition(tagStart);
        } else {
            cursor.setPosition(tagEnd);
        }

        m_internalOperation = true;
        setTextCursor(cursor);
        m_internalOperation = false;
    }
}

void SkillCommandTextEdit::onThemeChanged()
{
    updateCharCountLabelPosition();
    update();
}

// ClickableLabel 实现
ClickableLabel::ClickableLabel(const QString &text, QWidget *parent)
    : DLabel(text, parent)
    , m_hovered(false)
{
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Insert it into the input field"));
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &ClickableLabel::onThemeChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::paletteTypeChanged,
            this, &ClickableLabel::onThemeChanged);
}

QSize ClickableLabel::sizeHint() const
{
    QSize baseSize = DLabel::sizeHint();
    return QSize(baseSize.width() + 8, baseSize.height() + 8);
}

void ClickableLabel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();

    QColor bgColor = palette.color(DPalette::Highlight);
    if (m_hovered) {
        bgColor.setAlphaF(0.3);
    } else {
        bgColor.setAlphaF(0.2);
    }

    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);

    painter.drawRoundedRect(rect(), 4, 4);

    QColor textColor = palette.color(DPalette::Highlight);
    painter.setPen(textColor);
    painter.setFont(font());
    painter.drawText(rect(), Qt::AlignCenter, text());
}

void ClickableLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    DLabel::mousePressEvent(event);
}

#ifdef COMPILE_ON_QT6
void ClickableLabel::enterEvent(QEnterEvent *event)
#else
void ClickableLabel::enterEvent(QEvent *event)
#endif
{
    m_hovered = true;
    update();
    DLabel::enterEvent(event);
}

void ClickableLabel::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    DLabel::leaveEvent(event);
}

void ClickableLabel::onThemeChanged()
{
    update();
}

bool SkillCommandTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    return source->hasText();
}

void SkillCommandTextEdit::insertFromMimeData(const QMimeData *source)
{
    QString pastedText = source->text();
    if (!source->hasText() || pastedText.isEmpty())
        return;


    bool hadExistingTag = SelectionTagObject::hasSelectionTag(document());

    QTextCursor cursor = textCursor();
    int cursorPos = cursor.position();
    QString currentText = toPlainText();

    // 检查粘贴后是否会超出字符限制
    QString beforeCursor = currentText.left(cursorPos);
    QString afterCursor = currentText.mid(cursorPos);
    QString resultText = beforeCursor + pastedText + afterCursor;

    if (resultText.length() > 500) {
        int availableSpace = 500 - beforeCursor.length() - afterCursor.length();
        if (availableSpace > 0) {
            pastedText = pastedText.left(availableSpace);
            setAlert(true);
            showAlertMessage(tr("Exceeded character limit"), -1);
        } else {
            qCWarning(logAIGUI) << "Cannot paste: no available space";
            setAlert(true);
            showAlertMessage(tr("Exceeded character limit"), -1);
            return;
        }
    }

    cursor.insertText(pastedText);

    m_placeholderVisible = false;
    m_userHasEdited = true;
    update();
}

void SkillCommandTextEdit::updateCharCountLabel()
{
    if (!m_pCharCountLabel) {
        qCWarning(logAIGUI) << "Character count label is null";
        return;
    }

    QString currentText = toPlainText();
    int charCount = currentText.length();
    m_pCharCountLabel->setText(QString("%1/500").arg(charCount));
}

void SkillCommandTextEdit::updateCharCountLabelPosition()
{
    if (!m_pCharCountLabel) {
        qCWarning(logAIGUI) << "Character count label is null during position update";
        return;
    }

    // 根据当前文本内容和字体动态调整标签大小
    QString currentText = m_pCharCountLabel->text();
    QFontMetrics fm(m_pCharCountLabel->font());
    int textWidth = fm.horizontalAdvance(currentText);
    int textHeight = fm.height();

    // 设置适当的padding
    int padding = 8;
    int labelWidth = textWidth + padding;
    int labelHeight = qMax(textHeight, 20); // 最小高度20

    m_pCharCountLabel->setFixedSize(labelWidth, labelHeight);

    // 将字数统计放在右下角
    int x = width() - labelWidth - 10;
    int y = height() - labelHeight - 5;
    m_pCharCountLabel->move(x, y);
    m_pCharCountLabel->raise();

    // 更新viewport边距为字符计数标签预留空间
    updateViewportMargins();
}

void SkillCommandTextEdit::updateViewportMargins()
{
    if (!m_pCharCountLabel) {
        qCWarning(logAIGUI) << "Character count label is null during viewport margins update";
        return;
    }

    // 计算需要预留的空间
    int labelHeight = m_pCharCountLabel->height();

    // 只为底部的字符计数标签预留空间（最后一行），不预留右边空间
    // 预留底部边距，让文本不会覆盖到字符计数标签
    int bottomMargin = labelHeight + 10; // 额外10px边距

    setViewportMargins(4, 4, 4, bottomMargin);
}

void SkillCommandTextEdit::onFontChanged(const QFont &font)
{
    updateCharCountLabelPosition();
    update();
}

bool SkillCommandTextEdit::isAlert() const
{
    return m_alertControl ? m_alertControl->isAlert() : false;
}

void SkillCommandTextEdit::setAlert(bool isAlert)
{
    if (m_alertControl) {
        m_alertControl->setAlert(isAlert);
    }
}

void SkillCommandTextEdit::showAlertMessage(const QString &text, int duration)
{
    if (m_alertControl) {
        m_alertControl->showAlertMessage(text, duration);
    }
}

void SkillCommandTextEdit::hideAlertMessage()
{
    if (m_alertControl) {
        m_alertControl->hideAlertMessage();
    }
}

