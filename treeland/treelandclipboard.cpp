#include "treelandclipboard.h"

#include <QBuffer>
#include <QGuiApplication>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logTreeland)
UOSAI_USE_NAMESPACE

TreelandClipboard::TreelandClipboard(QObject *parent) : BaseClipboard(parent)
{
    m_manager.reset(new DataControlDeviceV1ManagerV1);
    QObject::connect(m_manager.get(), &DataControlDeviceV1ManagerV1::activeChanged, this, [this]() {
        if (m_manager->isActive()) {
            qCDebug(logTreeland) << "Data control manager activated";
            auto waylandApp = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
            if (!waylandApp) {
                qCWarning(logTreeland) << "Failed to get wayland application interface";
                return;
            }
            auto seat = waylandApp->seat();

            if (!seat)
                qFatal("Failed to get wl_seat frome QtWayland QPA!");

            m_device.reset(new DataControlDeviceV1(m_manager->get_data_device(seat)));

            connect(m_device.get(), &DataControlDeviceV1::receivedSelectionChanged, this, [this]() {
                if (!m_device->selection()) {
                    qCDebug(logTreeland) << "Clipboard selection changed";
                    Q_EMIT changed(QClipboard::Clipboard);
                    // Q_EMIT selectWords();

                    if (m_device->m_receivedSelection) {
                        DataControlOfferV1 *offer = m_device->m_receivedSelection.get();
                        for (auto type : offer->formats()) {
                            if(type != "text/plain;charset=utf-8") continue;

                            QVariant data = offer->retrieveData(type);
                            qCDebug(logTreeland) << "Retrieved clipboard data of type:" << type;
                            clipText = data.toString();
                            if (isScribeWordsVisible() && type == "text/plain;charset=utf-8") {
                                qCDebug(logTreeland) << "Emitting selectWords signal";
                                Q_EMIT selectWords();
                            }
                        }
                    }
                }
            });
            connect(m_device.get(), &DataControlDeviceV1::selectionChanged, this, [this]() {
                Q_EMIT changed(QClipboard::Clipboard);
                Q_EMIT selectWords();
            });

            connect(m_device.get(),
                    &DataControlDeviceV1::receivedPrimarySelectionChanged,
                    this,
                    [this]() {
                        if (!m_device->primarySelection()) {
                            Q_EMIT changed(QClipboard::Selection);

                            if (m_device->m_receivedPrimarySelection) {
                                DataControlOfferV1 *offer =
                                    m_device->m_receivedPrimarySelection.get();
                                for (auto type : offer->formats()) {
                                    if(type != "text/plain;charset=utf-8") continue;

                                    QVariant data = offer->retrieveData(type);
                                    // qWarning() << "----------"
                                    //               "receivedPrimarySelectionChanged"
                                    //            << "type:=" << type << "data:= " << data;
                                    clipText = data.toString();
                                    if (isScribeWordsVisible() && type == "text/plain;charset=utf-8")
                                        Q_EMIT selectWords();
                                }
                            }
                        }
                    });
            connect(m_device.get(), &DataControlDeviceV1::primarySelectionChanged, this, [this]() {
                Q_EMIT changed(QClipboard::Selection);
                Q_EMIT selectWords();
            });

        } else {
            m_device.reset();
        }
    });

    m_manager->instantiate();
}

bool TreelandClipboard::isValid()
{
    bool valid = m_manager && m_manager->isInitialized();
    qCDebug(logTreeland) << "Clipboard validity check:" << valid;
    return valid;
}

QString TreelandClipboard::getClipText()
{
    return clipText;
}

void TreelandClipboard::clearClipText()
{
    clipText = "";
}

void TreelandClipboard::setClipText(const QString &text)
{
    clipText = text;
}

bool TreelandClipboard::isScribeWordsVisible()
{
    return !clipText.trimmed().isEmpty();
}

void TreelandClipboard::blockChangedSignal(bool block)
{
    m_device->blockSignals(block);
}