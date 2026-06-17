#ifndef UPDATELOGDIALOG_H
#define UPDATELOGDIALOG_H

#include <DDialog>
#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>
#include <DScrollArea>
#include <DGuiApplicationHelper>
#include <DIconButton>
#include <DFrame>

#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QStringList>

namespace uos_ai {

struct UpdateLogItem {
    QString type;        // "new", "improvement", "fix"
    QString description;
    QStringList descriptionImages;
};

struct UpdateLogVersion {
    QString version;
    QString date;
    QList<UpdateLogItem> items;
};

class UpdateLogVersionCard : public DTK_WIDGET_NAMESPACE::DFrame
{
public:
    explicit UpdateLogVersionCard(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
};

class UpdateLogDialog : public DTK_WIDGET_NAMESPACE::DDialog
{
    Q_OBJECT
public:
    explicit UpdateLogDialog(QWidget *parent = nullptr);

private slots:
    void onUpdateSystemTheme(const DTK_GUI_NAMESPACE::DGuiApplicationHelper::ColorType &);

private:
    void initUI();
    void initConnect();
    void loadUpdateLogs();
    static bool isSupportedDescriptionImage(const QString &path);
    static QString updateLogImagePath(const QString &path);
    static QStringList parseDescriptionImages(const QJsonValue &value);
    static void parseDescription(const QJsonValue &descriptionValue, QString *description, QStringList *images);
    QHBoxLayout* createVersionWidget(const UpdateLogVersion &version);
    QHBoxLayout* createItemWidget(const UpdateLogItem &item, int index);
    
private:
    DTK_WIDGET_NAMESPACE::DScrollArea *m_scrollArea;
    DTK_WIDGET_NAMESPACE::DWidget *m_contentWidget;
    QVBoxLayout *m_contentLayout;
    
    QList<UpdateLogVersion> m_updateLogs;
};

} // namespace uos_ai

#endif // UPDATELOGDIALOG_H
