#ifndef DRAGMONITOR_H
#define DRAGMONITOR_H

#include <QObject>

namespace uos_ai {

class DragMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DragMonitor(QObject *parent = nullptr);

signals:
    void dragEnter(const QStringList &urls);
protected slots:
   void onDragNotify(const QStringList &urls);
};

}

#endif // DRAGMONITOR_H
