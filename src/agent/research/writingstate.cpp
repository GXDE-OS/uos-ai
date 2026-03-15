#include "writingstate.h"
#include "tools/researchtools.h"

#include <QJsonDocument>

namespace uos_ai {

QJsonObject WritingState::toJson() const
{
    QJsonObject obj;
    obj["stage"]           = static_cast<int>(m_stage);
    obj["outline"]         = m_outline;
    obj["article_content"] = m_articleContent;
    return obj;
}

WritingState WritingState::fromJson(const QJsonObject &json)
{
    WritingState state;
    state.m_stage          = static_cast<Stage>(json.value("stage").toInt(0));
    state.m_outline        = json.value("outline").toObject();
    state.m_articleContent = json.value("article_content").toString();
    return state;
}

WritingState WritingState::fromParams(const QVariantHash &params)
{
    QString jsonStr = params.value("writing_state_json").toString();
    if (!jsonStr.isEmpty()) {
        QJsonObject obj = QJsonDocument::fromJson(jsonStr.toUtf8()).object();
        if (!obj.isEmpty())
            return fromJson(obj);
    }
    return WritingState();
}

QString WritingState::toJsonString() const
{
    return QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
}

QString WritingState::buildContextMessage() const
{
    switch (m_stage) {
    case Stage::GenerateOutline:
        return QString();
    case Stage::GenerateArticle: {
        QString outlineMd;
        ResearchTools::outlineJson2Md(m_outline, 1, outlineMd);
        return QString("[WRITING_PROJECT]\nstage: researching\noutline:\n%1\n[/WRITING_PROJECT]")
                .arg(outlineMd);
    }
    case Stage::AdjustArticle:
        return QString("[WRITING_PROJECT]\nstage: adjusting\n[/WRITING_PROJECT]");
    }
    return QString();
}

} // namespace uos_ai
