# 自动查找当前目录下的所有源文件
file(GLOB AGENT_SRCS 
    "*.cpp"
    "*.h"
)

# 可选：排除不需要的文件（如果有）
# list(REMOVE_ITEM AGENT_SRCS "some_file_to_exclude.cpp")

# 添加源文件到项目
target_sources(${PROJECT_NAME} PRIVATE ${AGENT_SRCS})
