#include "outlinemode.h"
#include "writingstage.h"
#include "writingworkspace.h"

namespace uos_ai {

QString OutlineMode::name() const
{
    return QStringLiteral("OutlineMode");
}

QList<WritingStage*> OutlineMode::createStages(const WritingWorkspace &ws,
                                                const ModelMessage &userMsg,
                                                const QString &userIntent,
                                                QObject *parent) const
{
    Q_UNUSED(userMsg)

    // Determine entry stage from intent or workspace state
    QString entry;
    if (!userIntent.isEmpty()) {
        entry = userIntent;
    } else {
        // Fallback: infer from workspace data state
        const Article *art = ws.activeArticle();
        if (ws.hasArticleContent())
            entry = "adjust";
        else if (art && art->hasOutline())
            entry = "research";
        else
            entry = "outline";
    }

    QList<WritingStage*> stages;

    if (entry == "chat") {
        stages.append(new ChatStage(parent));
        return stages;
    }

    if (entry == "adjust") {
        stages.append(new AdjustStage(parent));
        return stages;
    }

    if (entry == "rollback") {
        stages.append(new RollbackStage(parent));
        return stages;
    }

    if (entry == "new_article") {
        stages.append(new NewArticleStage(parent));
        stages.append(new OutlineStage(parent));
        return stages;
    }

    if (entry == "rewrite") {
        // Full rewrite: outline -> research -> write -> predict
        stages.append(new OutlineStage(parent));
        stages.append(new ResearchStage(parent));
        stages.append(new ComposeStage(parent));
        stages.append(new PredictStage(parent));
        return stages;
    }

    // Full pipeline stages in order
    QStringList pipeline = {"outline", "research", "write", "predict"};

    int startIdx = pipeline.indexOf(entry);
    if (startIdx < 0)
        startIdx = 0;

    for (int i = startIdx; i < pipeline.size(); ++i) {
        const QString &id = pipeline[i];
        if (id == "outline")
            stages.append(new OutlineStage(parent));
        else if (id == "research")
            stages.append(new ResearchStage(parent));
        else if (id == "write")
            stages.append(new ComposeStage(parent));
        else if (id == "predict")
            stages.append(new PredictStage(parent));
    }

    return stages;
}

} // namespace uos_ai
