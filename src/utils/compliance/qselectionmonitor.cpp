#include "qselectionmonitor.h"
#include "atspidesktop.h"
#include "util.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QDir>

UOSAI_USE_NAMESPACE

QSelectionMonitor::QSelectionMonitor(QObject *parent) : QThread(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    connect(QGuiApplication::clipboard(),&QClipboard::selectionChanged,this,&QSelectionMonitor::selectionChanged);
    connect(&m_checkTimer, &QTimer::timeout, this, [this](){
        this->start();
    });

    m_checkTimer.start(5000);
}

QSelectionMonitor* QSelectionMonitor::getInstance()
{
    static QSelectionMonitor instance;
    return &instance;
}

void QSelectionMonitor::selectionChanged()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString originalText = clipboard->text(QClipboard::Selection);
    originalText = originalText.trimmed();
    if (originalText.isEmpty())
        return;

    int seletedPid = AtspiDesktop::getInstance()->getSelectedProgramPId();
    m_lock.lock();
    m_mapStore[QString::number(seletedPid)] = originalText;
    m_lock.unlock();
}

QString QSelectionMonitor::getCurSelText()
{
    QMutexLocker locker(&m_lock);
    int seletedPid = AtspiDesktop::getInstance()->getSelectedProgramPId();
    return m_mapStore[QString::number(seletedPid)];
}

void QSelectionMonitor::run()
{
    m_lock.lock();
    for (auto iter = m_mapStore.begin();iter != m_mapStore.end();) {
        QDir dir(QString("/proc/") + iter.key());
        if(!dir.exists()){
            iter = m_mapStore.erase(iter);
            continue;
        }
        ++iter;
    }
    //qDebug() << "============" << m_mapStore << QThread::currentThread() << qApp->thread()<< this->thread();
    m_lock.unlock();
}
