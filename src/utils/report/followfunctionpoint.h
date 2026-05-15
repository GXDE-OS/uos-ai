#ifndef FOLLOWFUNCTION_POINT_H
#define FOLLOWFUNCTION_POINT_H

#include "basicpoint.h"
#include "wordwizard.h"

namespace uos_ai {

namespace report {

class FollowFunctionPoint : public BasicPoint
{
public:
    explicit FollowFunctionPoint(int type) : BasicPoint()
    {
        this->m_eventId.second = EventID::FOLLOWALONG_FUNCTION;
        this->m_event = "followalong_function";

        QString text;
        switch (type) {
        case WordWizard::WIZARD_TYPE_SEARCH:
            text = "search";
            break;
        case WordWizard::WIZARD_TYPE_EXPLAIN:
            text = "explain";
            break;
        case WordWizard::WIZARD_TYPE_SUMMARIZE:
            text = "summary";
            break;
        case WordWizard::WIZARD_TYPE_TRANSLATE:
            text = "translate";
            break;
        case WordWizard::WIZARD_TYPE_RENEW:
            text = "continue writing";
            break;
        case WordWizard::WIZARD_TYPE_EXTEND:
            text = "expand";
            break;
        case WordWizard::WIZARD_TYPE_CORRECT:
            text = "correct";
            break;
        case WordWizard::WIZARD_TYPE_POLISH:
            text = "polish";
            break;
        case WordWizard::WIZARD_TYPE_KNOWLEDGE:
            text = "add to knowledge base";
            break;
        }
        QVariantMap map;
        map.insert("function", text);
        this->setAdditionalData(map);
    }
    ~FollowFunctionPoint() {}
};

}

}

#endif // FOLLOWFUNCTION_POINT_H
