#include "chatfixedresponse.h"

#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QLocale>

const QVector<QString> &filterWords = {
    "你是谁",
    "你的名字是什么",
    "你由谁创建",
    "你是一个人工智能吗",
    "你可以做什么",
    "你有什么用途",
    "whoareyou",
    "whatisyourname",
    "whocreatedyou",
    "areyouanai",
    "whatcanyoudo",
    "whatisyourpurpose"
};

QString ChatFixedResponse::checkRequestText(const QString &text)
{
    QString detectedText = text;;
    const QJsonDocument &document = QJsonDocument::fromJson(text.toUtf8());
    if (document.isArray()) {
        QJsonArray textArray = document.array();
        if (!textArray.isEmpty() && textArray.last()["role"].toString() == "user") {
            detectedText = textArray.last()["content"].toString();
        }
    }

    if (detectedText.length() >= 30)
        return QString();

    detectedText = detectedText.remove(QRegExp("[^\u4e00-\u9fa5a-zA-Z]+"));
    if (filterWords.contains(detectedText.simplified().toLower())) {
        if (QLocale::Chinese != QLocale::system().language() || QLocale::SimplifiedChineseScript != QLocale::system().script()) {
            return "Hello, I am your personal assistant UOS AI, a desktop intelligent assistant developed by Tongxin Software."
                   "I integrate a variety of large models into one, and you can choose different large models to serve you. "
                   "You can communicate with me via text or voice, and I can answer your questions, provide useful tips and suggestions, "
                   "and generate images based on your descriptions. At the same time, "
                   "I am deeply integrated with UOS and can help you complete system operations, such as adjusting screen brightness, opening applications, "
                   "creating schedules, etc. Is there anything I can do to help you?\n"
                   "You can ask me:\n"
                   "1. Help me write a work daily report template\n"
                   "2. How to write the summation formula in Excel?\n"
                   "3. Help me adjust the computer screen brightness to 70%\n";
        } else {
            return "您好，我是您的私人助理UOS AI，是由统信软件开发的一款桌面智能助手。 我集多种大模型于一体，您可以选择不同大模型为您服务。"
                   "您可以用文字或者语音与我交流，我可以回答您的问题、提供有用的提示与建议、可以根据您的描述生成图片。同时我与UOS深度融合，"
                   "可以帮您完成系统操作，如调整屏幕亮度、打开应用、建立日程等。 请问我有什么可以帮助您的吗？\n"
                   "您可以问我：\n"
                   "1、帮我写个工作日报模板\n"
                   "2、Excel中的求和公式如何写？\n"
                   "3、帮我电脑屏幕亮度调整到70%";
        }
    }

    return QString();
}
