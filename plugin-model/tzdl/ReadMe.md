# 1.配置文件存放路径

    1.1 配置文件存放于：~/.config/deepin/uos-ai-assistant/plugin-model
    1.2 配置文件名称为“TzdlHttpServerConfig.json”
    1.3 可添加多个配置文件，每个配置文件对应一个智能体

# 2.配置文件内容

    2.1 配置文件内容为 json 格式
    2.2 配置文件内容如下：
    {
        "agentCode": "e36a0ab1-b673-49d0-9534-1480a1f47c91",
        "agentLlmConfig": {
            "agentDisplayName": "智能体",
            "agentIcon": "tzdl",
            "description": "这是智能体",
            "iconPrefix": "/home/jianghanxiao/Project/gerrit/snipe-2.0-workspace-bugfix/uos-ai/plugin-model/tzdl/assets/icons/",
            "llmDisplayName": "LLM",
            "llmIcon": "uoslm"
        },
        "agentVersion": "170686385634",
        "clearSessionRoute": "/sfm-gateway-qypcw/sfm-api-gateway/gateway/agent/api/clearSession",
        "createSessionRoute": "/sfm-gateway-qypcw/sfm-api-gateway/gateway/agent/api/createSession",
        "runSessionRoute": "/sfm-gateway-qypcw/sfm-api-gateway/gateway/agent/api/run",
        "serverRootUrl": "http://localhost:8080",
        "tokenID": "YOUR TOKEN_ID"
    }
# 3.配置文件说明

    3.1 agentCode：智能体编码，对应接口文档中的智能体编码
    3.2 agentVersion：智能体版本，对应接口文档中的智能体版本
    3.3 clearSessionRoute：清除 session 的路由地址
    3.4 createSessionRoute：创建 session 的路由地址
    3.5 runSessionRoute：发起 agent 调用的路由地址
    3.6 serverRootUrl：服务器根地址
    3.7 tokenID：令牌 ID，用于验证代理的身份
    3.8 agentLlmConfig：智能体配置，包括智能体名称、智能体描述、智能体图标、LLM 名称和 LLM 图标
        3.8.1 agentDisplayName：智能体名称
        3.8.2 agentIcon：智能体图标，图标后缀为“.svg”，此处不用添加后缀
        3.8.3 description：智能体描述
        3.8.3 iconPrefix：图标前缀，即图标存放的路径
        3.8.4 llmDisplayName：LLM 名称
        3.8.5 llmIcon：LLM 图标，图标后缀为“.svg”，此处不用添加后缀
