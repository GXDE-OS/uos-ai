#ifndef OUTLINEMODE_H
#define OUTLINEMODE_H

#include "writingmode.h"

namespace uos_ai {

/**
 * OutlineMode: 大纲式写作模式
 *
 * 管线：OutlineStage → ResearchStage → ComposeStage → PredictStage
 * 特殊路径：AdjustStage、ChatStage
 */
class OutlineMode : public WritingMode
{
public:
    QString name() const override;
    QList<WritingStage*> createStages(const WritingWorkspace &ws,
                                      const ModelMessage &userMsg,
                                      const QString &userIntent,
                                      QObject *parent) const override;
};

} // namespace uos_ai

#endif // OUTLINEMODE_H
