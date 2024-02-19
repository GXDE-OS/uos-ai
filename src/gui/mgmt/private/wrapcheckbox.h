#ifndef WRAPCHECKBOX_H
#define WRAPCHECKBOX_H

#include <QWidget>

#include <DCheckBox>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class WrapCheckBox : public QWidget
{
    Q_OBJECT

public:
    explicit WrapCheckBox(QWidget *parent = nullptr);

    WrapCheckBox(const QString &text, QWidget *parent = nullptr);

    Qt::CheckState checkState() const;
    void setCheckState(Qt::CheckState state);

    void setText(const QString &text);
    QString text() const;

    Qt::TextFormat textFormat() const;
    void setTextFormat(Qt::TextFormat);

    bool openExternalLinks() const;
    void setOpenExternalLinks(bool open);

    void setTextInteractionFlags(Qt::TextInteractionFlags flags);
    Qt::TextInteractionFlags textInteractionFlags() const;

    // 文本自动换行长度不固定，可能很短就自动换行了，固定控件长度可以解决
    void setTextMaxWidth(int);

    void setDisabled(bool);
private:
    void initUI();

    void initConnect();

signals:
    void stateChanged(int);

private slots:
    void onUpdateSystemFont(const QFont &);

private:
    DCheckBox *m_checkBox{nullptr};
    DLabel *m_label{nullptr};
    QString m_text;
    int m_maxWidth;
};

#endif // WRAPCHECKBOX_H
