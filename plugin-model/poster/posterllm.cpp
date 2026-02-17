// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "posterllm.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QtDBus>

#define NM_SERVICE      "org.freedesktop.NetworkManager"
#define NM_PATH         "/org/freedesktop/NetworkManager"
#define NM_INTERFACE    "org.freedesktop.NetworkManager"

using namespace uos_ai;

enum NMState {
    NM_STATE_UNKNOWN = 0,
    NM_STATE_ASLEEP = 10,
    NM_STATE_DISCONNECTED = 20,
    NM_STATE_DISCONNECTING = 30,
    NM_STATE_CONNECTING = 40,
    NM_STATE_CONNECTED_LOCAL = 50,
    NM_STATE_CONNECTED_SITE = 60,
    NM_STATE_CONNECTED_GLOBAL = 70
};

static QByteArray generateHmac(QCryptographicHash::Algorithm algorithm, const QByteArray &data, const QByteArray &key)
{
    // 填充密钥以匹配块大小
    QByteArray keyPadded(64, 0x00);
    for (int i = 0; i < key.length(); ++i) {
        keyPadded[i] = key[i];
    }

    // 创建 oKeyPad 和 iKeyPad
    QByteArray oKeyPad = keyPadded;
    QByteArray iKeyPad = keyPadded;
    for (int i = 0; i < 64; ++i) {
        oKeyPad[i] = oKeyPad[i] ^ 0x5c;
        iKeyPad[i] = iKeyPad[i] ^ 0x36;
    }

    // 计算 HMAC
    QCryptographicHash hash(algorithm);
    hash.addData(iKeyPad);
    hash.addData(data);
    QByteArray innerHash = hash.result();

    hash.reset();
    hash.addData(oKeyPad);
    hash.addData(innerHash);
    return hash.result();
}

PosterLLM::PosterLLM() : QObject(), LLMModel()
{
  auto configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
  configPath = configPath
          + "/" + qApp->organizationName()
          + "/" + qApp->applicationName()
          + "/" + qApp->applicationName() + "-plugin.conf";

  QFileInfo configFile(configPath);
  if (!configFile.exists()) {
      //生成文件
      QFile file(configPath);
      file.open(QFile::NewOnly);
      file.close();
      QSettings* set = new QSettings(configPath, QSettings::IniFormat);
      set->beginGroup("Poster");
      set->setValue("account", "tongxintestjfwybd342hjwq");
      set->endGroup();
      key = "tongxintestjfwybd342hjwq";
      qInfo() << "create conf " << configPath;
  } else {
      QSettings* set = new QSettings(configPath, QSettings::IniFormat);
      // 检查[Poster]段落是否存在
      if (set->childGroups().contains("Poster")) {
          // [Poster]段落存在，读取account值
          key = set->value("Poster/account","tongxintestjfwybd342hjwq").toString();
          qInfo() << "Existing account:" << key;
      } else {
          // [Poster]段落不存在，创建该段落并设置account值
          set->beginGroup("Poster");
          set->setValue("account", "tongxintestjfwybd342hjwq");
          set->endGroup();
          key = "tongxintestjfwybd342hjwq";
      }
  }
}

QString PosterLLM::model() const
{
    return modelID();
}

QJsonObject PosterLLM::generate(const QString &content, const QVariantHash &params, LLMModel::streamFuncion stream, void *user)
{
    /* 1. 调用接口请求模型
     * 2. 流式接收数据通过stream(data, user); 非
     * 3. 流式添加到返回的Json对象的 “GENERATE_RESPONSE_CONTENT”字段中
     * 4. 组装回复JSON
     *      QJsonObject response;
            response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
            response.insert(GENERATE_RESPONSE_CODE, errorPair.first);
            response.insert(GENERATE_RESPONSE_ERRORMSG, errorPair.second);
     */

    QDBusMessage msg = QDBusMessage::createMethodCall(NM_SERVICE,
                                                      NM_PATH,
                                                      NM_INTERFACE,
                                                      "state");

    QDBusReply<quint32> ret = QDBusConnection::systemBus().call(msg);
    if (ret.isValid() && ret.value() == NM_STATE_CONNECTED_GLOBAL) {
      qCritical() << "network is online";
    } else {
      QEventLoop loop;
      QTimer::singleShot(1000, [&loop](){
          loop.quit();
      });
      loop.exec();
      QString responseContent;
      stream("连接失败，请检查是否连接了互联网，或稍后再试。", user);
      responseContent = "连接失败，请检查是否连接了互联网，或稍后再试。";
      QJsonObject response;
      response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
      response.insert(GENERATE_RESPONSE_CODE, 0);
      response.insert(GENERATE_RESPONSE_ERRORMSG, "");
      return response;
    }

    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    QJsonArray array = doc.array();
    QJsonValue value = array.last();
    QJsonObject obj = value.toObject();
    QString lastContent = obj["content"].toString();
    qInfo()<<"Poster Assistant content: "<<lastContent;

    QNetworkAccessManager manager;

    QUrl url("https://album.photosir.cn/tongxin/api/v1/ai/completions");
    QNetworkRequest request(url);
    QJsonObject jsonObject;
    jsonObject["key"] = key;
    jsonObject["content"] = lastContent;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 将JSON对象转换为字符串
    QJsonDocument jsonDoc(jsonObject);
    QByteArray jsonData = jsonDoc.toJson();

    // 发送POST请求
    QNetworkReply *reply = manager.post(request, jsonData);

    QString responseContent;
    QEventLoop loop;
    connect(this, &PosterLLM::sigAbort, this, [&](){
        reply->abort();
        responseContent = "task cancel";
    });
    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        const QByteArray &data = reply->readAll();
        QJsonObject obj = QJsonDocument::fromJson(data).object();
        if (obj.value("code").toInt() == 0) {
            responseContent = getContent(obj, stream, user);
        }
    }
    QJsonObject response;
    response.insert(GENERATE_RESPONSE_CONTENT, responseContent);
    response.insert(GENERATE_RESPONSE_CODE, 0);
    response.insert(GENERATE_RESPONSE_ERRORMSG, "");
    return response;
}

QString PosterLLM::getContent(QJsonObject object, LLMModel::streamFuncion stream, void *user)
{
    QString content_result ="";
    QString firstLine = "根据您的描述，为您准备的海报内容如下：<br>";
    stream(firstLine, user);
    content_result += firstLine;

    QString maintitle = "主标题：" + object.value("data").toObject().value("maintitle").toString() + "<br>";
    stream(maintitle, user);
    content_result += maintitle;

    QString subtitle = "副标题：" + object.value("data").toObject().value("subtitle").toString() + "<br>";
    stream(subtitle, user);
    content_result += subtitle;

    QString gcid = "ID：" + object.value("data").toObject().value("gcid").toString() + "<br>";
    stream(gcid, user);
    content_result += gcid;

    QString prompt = "描述词：" + object.value("data").toObject().value("prompt").toString() + "<br>";
    stream(prompt, user);
    content_result += prompt;

    QString width = "宽：" + QString::number(object.value("data").toObject().value("width").toInt()) + "<br>";
    stream(width, user);
    content_result += width;

    QString height = "高：" + QString::number(object.value("data").toObject().value("height").toInt()) + "<br>";
    stream(height, user);
    content_result += height;
    return content_result;
}

void PosterLLM::setAbort()
{
    emit sigAbort();
}
