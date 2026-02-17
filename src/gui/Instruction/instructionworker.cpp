#include "instructionworker.h"
#include "uosai_global.h"

UOSAI_USE_NAMESPACE

InstructionWorker::InstructionWorker()
{

}

InstructionWorker::~InstructionWorker()
{

}

bool InstructionWorker::isAvailable(AssistantType assis, LLMChatModel model) const
{
    return availableAssis.contains(assis)
            && (availableManu.contains(LLMServerProxy::llmManufacturer(model))
            || availableLlm.contains(model));
}

QString InstructionWorker::getTagName() const
{
    return tagName;
}

QString InstructionWorker::getTagNameEn() const
{
    return tagNameEn;
}

QString InstructionWorker::getContent() const
{
    return defaultContent;
}

QString InstructionWorker::getDescription() const
{
    return description;
}
