#ifndef RAIDERSBUTTON_H
#define RAIDERSBUTTON_H

#include "uosfreeaccounts.h"

#include <DWidget>

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QFutureWatcher>
#include <QtConcurrent>

#include <DPalette>
#include <DGuiApplicationHelper>
#include <DFontSizeManager>

namespace uos_ai {

class RaidersButton: public QWidget
{
    Q_OBJECT
public:
    explicit RaidersButton(QWidget *parent = nullptr);

    void setText(const QString &);

    void setUrl(const QString &);

    void resetUrl();

private slots:
    void onUpdateSystemFont(const QFont &);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString m_text;
    QString m_url;
    QDateTime m_lastDateTime;
    UosFreeAccountActivity m_hasActivity;
    QSharedPointer<QFutureWatcher<QNetworkReply::NetworkError>> m_watcher;
};
}
#endif // RAIDERSBUTTON_H
