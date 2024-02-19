#ifndef PGSPARSER_H
#define PGSPARSER_H

#include <QString>
#include <QVector>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class PgsParser
{
public:

    PgsParser()
    {
        m_isValid.reserve(200);
        m_resultVec.reserve(200);
    }

    bool isFinished() const
    {
        return m_last;
    }

    const QString &appendResult(const QJsonObject &rslt)
    {
        if (m_last)
            return m_curText;

        QJsonObject textObject = rslt;

        if (!textObject.contains("sn"))
            return m_curText;

        int sn = textObject["sn"].toInt();
        if (!textObject.contains("pgs")) {
            return m_curText;
        }

        QString pgsType = textObject["pgs"].toString();
        if (!(pgsType == "apd" || pgsType == "rpl"))
            return m_curText;
        std::pair<int, int> rplRange;

        bool isLast = false;
        if (!(textObject.contains("ls")
                && textObject["ls"].isBool())) {
            return m_curText;
        }
        isLast = textObject["ls"].toBool();

        if (pgsType == "rpl") {
            if (!(textObject.contains("rg")
                    && textObject["rg"].isArray())) {
                return m_curText;
            }
            auto rgArray = textObject["rg"].toArray();
            if (2 != rgArray.size())
                return m_curText;
            rplRange.first = rgArray[0].toInt();
            rplRange.second = rgArray[1].toInt();
        }

        QString content;
        if (textObject.contains("ws")
                && textObject["ws"].isArray()) {
            auto wordArray = textObject["ws"].toArray();
            for (int i = 0; i < wordArray.size(); ++i) {
                auto cwObject = wordArray[i].toObject();
                if (!(cwObject.contains("cw")
                        && cwObject["cw"].isArray()))
                    return m_curText;
                auto cwArray = cwObject["cw"].toArray();
                for (int j = 0; j < cwArray.size(); ++j) {
                    auto cw = cwArray[j].toObject();
                    if (!(cw.contains("w")
                            && cw["w"].isString())) {
                        return m_curText;
                    }
                    content += cw["w"].toString();
                }

            }
        }

        if (!(content.isNull() || content.isEmpty())) {
            append(sn, pgsType, content, rplRange, isLast);
        }
        return m_curText;
    }

    const QString &getText()
    {
        return m_curText;
    }

    void clear()
    {
        m_resultVec.clear();
        m_isValid.clear();
        m_curText.clear();
        m_last = false;
    }

private:
    struct Result {
        enum ResultType {
            APPEND,
            REPLACE
        };

        int         sn{ -1 };
        QString     content;
        ResultType  type{ APPEND };
    };

    void append(int sn, const QString &type,
                const QString &content, const std::pair<int, int> &range, bool isLast = false)
    {
        // pgs sn start from 1
        if (sn - 1 >= m_resultVec.size()) {
            m_resultVec.resize(sn);
            m_isValid.resize(sn);
        }

        Result &result = m_resultVec[sn - 1];
        m_isValid[sn - 1] = 1;
        result.content = content;
        result.sn = sn;

        if (type == "apd") {
            result.type = Result::APPEND;
        } else {
            result.type = Result::REPLACE;
        }

        if (type == "rpl") {
            for (int i = range.first; i <= range.second; ++i) {
                m_isValid[i - 1] = 0;
            }
        }

        m_curText.clear();
        for (int i = 0; i < m_isValid.size(); ++i) {
            if (m_isValid[i]) {
                m_curText += m_resultVec[i].content;
            }
        }

        if (isLast) {
            qDebug() << "pgs final result:" << m_curText;
            m_last = true;
        }
    }

private:
    QVector<Result> m_resultVec;
    QVector<int>    m_isValid;
    QString         m_curText;
    bool            m_last{ false };
};


#endif // PGSPARSER_H
