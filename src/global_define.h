#ifndef GLOBAL_DEFINE_H
#define GLOBAL_DEFINE_H

inline constexpr char kDefaultAgentName[] = "default-agent";

#ifdef COMPILE_ON_V25
inline constexpr char kApplicationIconName[] = "UosAiAssistant";
#else
inline constexpr char kApplicationIconName[] = "uos-ai-assistant";
#endif

#endif // GLOBAL_DEFINE_H
