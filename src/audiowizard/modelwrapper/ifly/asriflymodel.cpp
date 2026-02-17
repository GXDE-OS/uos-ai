// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "asriflymodel.h"
#include "iatiflymodel.h"

#include <QRegularExpression>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

AsrIflyModel::AsrIflyModel(const QString &id, QObject *parent)
    :  AsrModel(id, parent)
{
    qCDebug(logAudioWizard) << "Initializing ASR iFly model with ID:" << id;
    connect(&decoder, SIGNAL(bufferReady()), this, SLOT(bufferReady()));
    connect(&decoder, SIGNAL(finished()), this, SLOT(bufferfinish()));
}

QString AsrIflyModel::startAsr(const QVariantMap &param)
{
    qCDebug(logAudioWizard) << "Starting ASR with parameters:" << param.keys();
    QString filePath = param["filePath"].toString();
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
#ifdef COMPILE_ON_QT6
    format.setSampleFormat(QAudioFormat::SampleFormat::Int16);
#else
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
#endif

#ifdef COMPILE_ON_QT6
    decoder.setSource(filePath); // 设置音频文件路径
#else
    decoder.setSourceFilename(filePath); // 设置音频文件路径
#endif

    decoder.setAudioFormat(format);
    if (m_iatModel == nullptr) {
        qCDebug(logAudioWizard) << "Creating new iFly IAT model instance";
        m_iatModel = new IatIflyModel();
        connect(m_iatModel, &IatIflyModel::textReceived, this, &AsrIflyModel::iatSlot, Qt::QueuedConnection);
    }
    
    qCDebug(logAudioWizard) << "Opening iFly server connection";
    m_iatModel->openServer();
    decoder.start(); // 开始解码
    return "000000";
}

void AsrIflyModel::bufferReady() {
    QAudioBuffer buffer = decoder.read();
    QByteArray pcmData;
    if (buffer.isValid()) {
        pcmData.append(static_cast<const char*>(buffer.constData<char>()), buffer.byteCount());
        qCDebug(logAudioWizard) << "Sending audio buffer to iFly, size:" << pcmData.size();
    } else {
        qCWarning(logAudioWizard) << "Received invalid audio buffer";
    }

    if (m_iatModel != nullptr) {
        m_iatModel->sendData(pcmData);
    }
}

void AsrIflyModel::bufferfinish() {
    if (m_iatModel != nullptr) {
        m_iatModel->inputEnd();
    }
}

void AsrIflyModel::iatSlot(QString result, bool finish)
{
    if (finish) {
        qCInfo(logAudioWizard) << "ASR completed with result:" << result.left(50) << "...";
        QJsonDocument doc;
        QJsonObject json = doc.object();
        json["code"] = "000000";
        json["failType"] = 0;
        json["status"] = 4;
        json["descInfo"] = "success";
        json["text"] = result;
        QString msg = QString(QJsonDocument(json).toJson());
        emit onNotify(msg);
        qCDebug(logAudioWizard) << "Emitting onNotify with result JSON";
    }
}

void AsrIflyModel::stopAsr()
{
    qCDebug(logAudioWizard) << "Stopping ASR processing";
    if (m_iatModel != nullptr) {
        m_iatModel->cancel();
        disconnect(m_iatModel,SIGNAL(textReceived(QString,bool)),this,SLOT(iatSlot(QString, bool)));
        delete m_iatModel;
        m_iatModel = nullptr;
    }
}

