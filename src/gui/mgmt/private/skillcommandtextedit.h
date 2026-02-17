#ifndef SKILLCOMMANDTEXTEDIT_H
#define SKILLCOMMANDTEXTEDIT_H
#include "uosai_global.h"
#include "selectiontagobject.h"

#include <DTextEdit>
#include <DLabel>
#include <DFontSizeManager>
#include <DAlertControl>

#include <QPaintEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTextCharFormat>
#include <QMimeData>
#include <QPaintEvent>

namespace uos_ai {

class SkillCommandTextEdit : public DTK_WIDGET_NAMESPACE::DTextEdit
{
    Q_OBJECT
public:
    explicit SkillCommandTextEdit(QWidget *parent = nullptr);
    void insertSelectedContentTag();
    void resetToDefault();
    QString getTextForSave() const; // 获取用于保存的文本（将有效标签转换为${}格式）
    void setTextFromSaved(const QString &text); // 从保存的文本设置内容（将${}格式转换为有效标签）

    bool isAlert() const;
    void setAlert(bool isAlert);
    void showAlertMessage(const QString &text, int duration = -1);
    void hideAlertMessage();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;

private slots:
    void onTextChanged();
    void onCursorPositionChanged();
    void onThemeChanged();
    void onFontChanged(const QFont &font);

signals:
    void tagDeleted();

private:
    void initUI();
    void initConnect();
    void setupDefaultContent();
    void setupSelectionTagHandler();
    void checkCursorPosition();
    void updateCharCountLabel();
    void updateCharCountLabelPosition();
    void updateViewportMargins();
    
private:
    QString m_plainTextPart;      // 明文部分
    QString m_placeholderPart;    // 暗文部分
    bool m_hasUserInput;          // 是否有用户输入
    bool m_placeholderVisible;    // 暗文是否可见
    QString m_userContent;        // 用户输入的内容
    bool m_isInitialized;         // 是否已初始化
    bool m_userHasEdited;         // 用户是否已经开始编辑（一旦编辑过就不再显示暗文）
    bool m_internalOperation;     // 是否正在进行内部操作（如格式化等）
    
    DTK_WIDGET_NAMESPACE::DLabel *m_pCharCountLabel;    // 字数统计标签
    DTK_WIDGET_NAMESPACE::DAlertControl *m_alertControl;
    SelectionTagObject *m_selectionTagObject;
};

// 可点击的标签，用于插入"{选中文字}"
class ClickableLabel : public DTK_WIDGET_NAMESPACE::DLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(const QString &text, QWidget *parent = nullptr);
    
    QSize sizeHint() const override;
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
#ifdef COMPILE_ON_QT6
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

signals:
    void clicked();

private:
    bool m_hovered;
};

}

#endif // SKILLCOMMANDTEXTEDIT_H 
