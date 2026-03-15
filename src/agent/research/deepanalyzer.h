#ifndef DEEPANALYZER_H
#define DEEPANALYZER_H

#include "llmagent.h"

namespace uos_ai {
class DeepAnalyzer : public LlmAgent
{
public:
    explicit DeepAnalyzer(QObject *parent = nullptr);
    virtual ~DeepAnalyzer() override;
};
}
#endif // DEEPANALYZER_H
