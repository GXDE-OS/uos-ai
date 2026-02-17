#include "asrwrapper.h"
#include "modelwrapper/ifly/asriflymodel.h"
#include <QString>
#include <QUuid>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

AsrWrapper::AsrWrapper(QObject *parent)
{
    m_currentID = QUuid::createUuid().toString();
    qCDebug(logAudioWizard) << "Creating ASR wrapper with ID:" << m_currentID;

    if (1) {
        //后期增加判断模型的接口
        qCDebug(logAudioWizard) << "Initializing iFly ASR model";
        m_asrModel = new AsrIflyModel(m_currentID, this);
        m_asrModel->setModel(AsrModel::ModelType::Ifly);
    }
    connect(m_asrModel,&AsrModel::onNotify,this,&AsrWrapper::onNotify,Qt::DirectConnection);
}
AsrWrapper::~AsrWrapper()
{

}

QString AsrWrapper::startAsr(const QVariantMap &param)
{
    qCDebug(logAudioWizard) << "Starting ASR with parameters:" << param.keys();
    return m_asrModel->startAsr(param);
}
void AsrWrapper::stopAsr()
{
    qCDebug(logAudioWizard) << "Stopping ASR";
    return m_asrModel->stopAsr();
}

