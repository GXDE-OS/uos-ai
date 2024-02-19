#include "tasmanager.h"
#include "tas.h"
#include "uossimpletas.h"

#include <QDebug>
#include <QVector>
#include <QRegExp>

const static QVector<QByteArray> gTasPuncts = {
    {"，"},
    {"。"},
    {"；"},
    {"！"},
    {"······"},
    {"……"}
};

const static QString gTasEnPuncts = {",.;!?"};

TasManager::TasManager()
    : QThread(nullptr)
    , m_tas(nullptr)
    , m_stopAuditing(false)
    , m_auditFinished(false)
    , m_result(nullptr)
{
    setTas(QSharedPointer<TAS>(new UosSimpleTas()));
    connect(this, &TasManager::sigReadyAuditContent, this, [&](const QByteArray & content) {
        m_mutex.lock();
        m_auditBuffQueue.append(content);
        m_condition.wakeAll(); // Notify waiting consumers
        m_mutex.unlock();
    });
    start();
}

TasManager::~TasManager()
{
    stopAuditing();
}

void TasManager::setTas(QSharedPointer<TAS> tas)
{
    m_tas = tas;
}

void TasManager::auditText(const QByteArray &text)
{
    for (const auto &ch : text) {
        m_auditingBuff.append(ch);
        if (matchPunct(ch)) {
            emit sigReadyAuditContent(m_auditingBuff);
            m_auditingBuff.clear();
        }
    }
}

void TasManager::endAuditText()
{
    if (!m_auditingBuff.isEmpty()) {
        emit sigReadyAuditContent(m_auditingBuff);
        m_auditingBuff.clear();
    }
}

QByteArray TasManager::getNormalText() const
{
    return m_auditingBuff;
}

bool TasManager::matchPunct(const QChar &ch)
{
    bool result = false;
    if (gTasEnPuncts.contains(ch)) {
        result = true;
    } else {
        for (const auto &pun : gTasPuncts) {
            if (m_auditingBuff.endsWith(pun)) {
                result = true;
                break;
            }
        }
    }
    return result ;
}

QString TasManager::clearPunct(const QByteArray &buff)
{
    auto newContent = QString(buff).replace(QRegExp("[,\\.;!\\?]"), "");
    for (const auto &pnt : gTasPuncts) {
        newContent.replace(pnt, "");
    }
    return newContent;
}

void TasManager::stopAuditing()
{
    m_stopAuditing.store(true);
    while (QThread::isRunning()) {
        m_condition.wakeAll();
        QThread::msleep(200);
    }
}

bool TasManager::auditFinished()
{
    return m_auditFinished.load();
}

QSharedPointer<TextAuditResult> TasManager::getResult()
{
    return m_result;
}

void TasManager::run()
{
    while (!m_stopAuditing.load()) {
        QSharedPointer<TextAuditResult> result
            = QSharedPointer<TextAuditResult>(new TextAuditResult());
        m_mutex.lock();
        if (m_auditBuffQueue.isEmpty()) {
            m_condition.wait(&m_mutex);
        }
        while (m_auditBuffQueue.size()) {
            result->code = TextAuditEnum::None;
            auto preAuditText = m_auditBuffQueue.dequeue();
            const auto &auditText = clearPunct(preAuditText);
            (*result.data()) = m_tas->doTextAuditing(auditText.toLocal8Bit().simplified());
            result->content =  preAuditText;
            emit sigAuditContentResult(result);
        }
        m_result = result;
        m_mutex.unlock();
        if (result->code != TextAuditEnum::None) {
            break;
        }
    }

    m_auditFinished.store(true);
}

