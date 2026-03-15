#include "researchagent.h"
#include "reportwriteragent.h"
#include "deepresearchagent.h"
#include "predictquestion.h"
#include "writingstate.h"
#include "networkdefs.h"
#include "reasoninguse.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

ResearchAgent::ResearchAgent(QObject *parent)
    : SequentialAgent(parent)
{
    m_name = "ResearchAgent";
    m_description = "An agent that performs comprehensive, multi-depth research and generates detailed reports.";
}

QSharedPointer<LlmAgent> ResearchAgent::create()
{
    return QSharedPointer<LlmAgent>(new ResearchAgent());
}

bool ResearchAgent::initialize()
{
    auto deepResearchAgent = new DeepResearchAgent(this);
    deepResearchAgent->initialize();
    m_subAgents["deep_research_agent"] = QSharedPointer<LlmAgent>(deepResearchAgent);
    m_agentOrder.append("deep_research_agent");
    connect(deepResearchAgent, &LlmAgent::readyReadChatDeltaContent, this, &ResearchAgent::readyReadChatDeltaContent);

    auto reportWriter = new ReportWriterAgent(this);
    reportWriter->initialize();
    m_subAgents["report_writer"] = QSharedPointer<LlmAgent>(reportWriter);
    m_agentOrder.append("report_writer");
    connect(reportWriter, &LlmAgent::readyReadChatDeltaContent, this, &ResearchAgent::readyReadChatDeltaContent);

    auto predictQuestion = new PredictQuestion(this);
    predictQuestion->initialize();
    m_subAgents["predict_question"] = QSharedPointer<LlmAgent>(predictQuestion);
    m_agentOrder.append("predict_question");
    connect(predictQuestion, &LlmAgent::readyReadChatDeltaContent, this, &ResearchAgent::readyReadChatDeltaContent);

    return true;
}

QJsonObject ResearchAgent::processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params)
{
    QJsonObject res;
    if (history.isEmpty()) {
        res = SequentialAgent::processRequest(question, history , params);
    } else {
        // 使用大纲生成前的question做为原始用户提问
        res = SequentialAgent::processRequest(history.first().toObject(), history , params);
    }

    if (lastError() != AIServer::NoError)
        ReasoningUse::reasoningUseTitle(this, "", ReasoningUse::Failed);

    return res;
}

bool ResearchAgent::beforeSubAgentCall(const QString &agentName, QJsonObject &currentQuestion, QJsonArray &localMessages, const QJsonArray &globalMessages)
{
    QJsonArray temp = std::move(localMessages);
    if (agentName == "deep_research_agent") {

    }

    if (agentName == "report_writer") {
        DeepResearchAgent *deepAgent = dynamic_cast<DeepResearchAgent*>(m_subAgents["deep_research_agent"].data());
        QJsonArray references;
        if (deepAgent)
            references = deepAgent->getReferences();

        currentQuestion.insert("references", references);
    }

    if (agentName == "predict_question") {
        QString report = currentQuestion.value("content").toString();
        QString initQuestion;
        if (!globalMessages.isEmpty()) {
            initQuestion = globalMessages.first().toObject().value("content").toString();
        }

        QString userPrompt = QString("User Original Task:%1\nFinal Content:\n%2").arg(initQuestion, report);
        QJsonObject ques;
        ques["role"] = "user";
        ques["content"] = userPrompt;

        currentQuestion = ques;
    }

    return true;
}

} // namespace uos_ai
