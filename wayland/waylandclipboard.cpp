#include "waylandclipboard.h"
#include <QMetaObject>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logWayland)

WaylandClipboard::WaylandClipboard(QObject *parent)
    : BaseClipboard(parent)
    , m_connectionThread(new QThread(this))
    , m_connectionThreadObject(new ConnectionThread())
    , m_eventQueue(nullptr)
    , m_dataControlDeviceManager(nullptr)
    , m_dataControlDevice(nullptr)
    , m_copyControlOffer(nullptr)
    , m_seat(nullptr)
{
    init();
}

WaylandClipboard::~WaylandClipboard()
{
    m_connectionThread->quit();
    m_connectionThread->wait();
    m_connectionThreadObject->deleteLater();
}

void WaylandClipboard::init()
{
    connect(m_connectionThreadObject, &ConnectionThread::connected, this, [this] {
        m_eventQueue = new EventQueue(this);
        m_eventQueue->setup(m_connectionThreadObject);

        Registry *registry = new Registry(this);
        setupRegistry(registry);
    }, Qt::QueuedConnection);
    m_connectionThreadObject->moveToThread(m_connectionThread);
    m_connectionThread->start();

    m_connectionThreadObject->initConnection();
}

void WaylandClipboard::setupRegistry(Registry *registry)
{
    connect(registry, &Registry::seatAnnounced, this,
    [this, registry](quint32 name, quint32 version) {
        m_seat = registry->createSeat(name, version, this);
    });

    connect(registry, &Registry::dataControlDeviceManagerAnnounced, this,
    [this, registry](quint32 name, quint32 version) {
        m_dataControlDeviceManager = registry->createDataControlDeviceManager(name, version, this);

        m_dataControlDevice = m_dataControlDeviceManager->getDataDevice(m_seat, this);
        // 线程池调整大一点，避免浏览器频繁触发，QtConcurrent::run 不再执行
        QThreadPool *pool = QThreadPool::globalInstance();
        pool->setMaxThreadCount(16);
        //监听 selectionOffered  当选择发生了变化，该信号会发出。需要监听clipbord 改变，监听 dataOffered 即可。
        connect(m_dataControlDevice, &DataControlDeviceV1::selectionOffered, [ = ](KWayland::Client::DataControlOfferV1 * offer) {
            if (offer) {
                m_copyControlOffer = m_dataControlDevice->primaryOfferedSelection();
                qCDebug(logWayland) << "New selection offer received";
            } else {
                qCWarning(logWayland) << "Null offer received";
            }

            if (!m_copyControlOffer) {
                qCDebug(logWayland) << "No valid copy control offer available";
                return;
            }

            qCDebug(logWayland) << "Available MIME types:" << m_copyControlOffer->offeredMimeTypes();
            clipText.clear();

            int pipeFds[2];
            if (pipe(pipeFds) != 0) {
                qCWarning(logWayland) << "Failed to create pipe:" << strerror(errno);
                return;
            }

            //根据mime类取数据，"text/plain" 是你需要的类型。
            m_copyControlOffer->receive("text/plain", pipeFds[1]);
            close(pipeFds[1]);

            QThreadPool *pool = QThreadPool::globalInstance();
            qDebug() << "Max thread: " << pool->maxThreadCount();
            qDebug() << "active thread: " << pool->activeThreadCount();
            QtConcurrent::run(
            [&] {
                QFile readPipe;
                if (readPipe.open(pipeFds[0], QIODevice::ReadOnly))
                {
                    // 此处可以获取到拷贝的文字
                    // 桌面智能助手需要在此处添加获取拷贝信息后的处理方式
                    //QMetaObject::invokeMethod((QObject*)this,"render",Qt::QueuedConnection, Q_ARG(QByteArray, readPipe.readAll()));
                    //emit copySelectContext(readPipe.readAll());
                    clipText = QString(readPipe.readAll());
                    qCDebug(logWayland) << "Retrieved clipboard text, length:" << clipText.length();
                    emit selectWords();
//                    qWarning() << "Pasted: " << clipText << clipText.length();
                }
                close(pipeFds[0]);
            });
        });
    });

    registry->setEventQueue(m_eventQueue);
    registry->create(m_connectionThreadObject);
    registry->setup();
}

QString WaylandClipboard::getClipText()
{
    return clipText;
}

void WaylandClipboard::clearClipText()
{
    clipText = "";
}

void WaylandClipboard::setClipText(const QString &text)
{
    clipText = text;
}

bool WaylandClipboard::isScribeWordsVisible()
{
    return !clipText.trimmed().isEmpty();
}

void WaylandClipboard::blockChangedSignal(bool block)
{
    m_dataControlDevice->blockSignals(block);
}
