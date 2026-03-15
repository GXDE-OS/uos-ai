#ifndef GLOBAL_DEFINE_H
#define GLOBAL_DEFINE_H

namespace uos_ai {

inline constexpr char kDefaultAgentName[] = "default-agent";

inline constexpr char kAIWritingMasterAgentName[]   = "WritingMasterAgent";
#ifdef COMPILE_ON_V25
inline constexpr char kApplicationIconName[] = "UosAiAssistant";
#else
inline constexpr char kApplicationIconName[] = "uos-ai-assistant";
#endif

}
#endif // GLOBAL_DEFINE_H
