#ifndef QSELECTIONMONITOR_H
#define QSELECTIONMONITOR_H

#include "uosai_global.h"

#include <QObject>
#include <QMap>
#include <QThread>
#include <QMutex>
#include <QTimer>

UOSAI_BEGIN_NAMESPACE

class QSelectionMonitor : public QThread
{
    Q_OBJECT
public:
    static QSelectionMonitor *getInstance();
    QString getCurSelText();
signals:

public slots:
    void selectionChanged();

private:
    void run() override;
    explicit QSelectionMonitor(QObject *parent = nullptr);
    QMap<QString,QString> m_mapStore;
    QMutex m_lock;
    QTimer m_checkTimer;
};

UOSAI_END_NAMESPACE

#endif // QSELECTIONMONITOR_H
