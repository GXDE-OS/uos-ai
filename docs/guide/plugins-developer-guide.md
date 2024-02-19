

# uos-ai助手API使用手册 v1.1



## 1. 获得大模型能力

### 1.1 接入流程

通过本方法，应用可以得到统一的大模型能力以开发应用自己的AI功能，应用在使用接口前需要完成以下步骤：

1. 连接公共dbus服务(接口见1.2)：

   service = com.deepin.copilot

   path = /com/deepin/copilot

   interface = com.deepin.copilot

2. 调用公共应用注册接口方法"registerApp"，返回是专属的dbus路径<path>，连接应用独立的专属dbus服务(接口见1.3)：

   service = com.deepin.copilot

   path = <path> 

   interface = com.deepin.copilot.app

3. 应用退出时，需要调用公共服务接口方法"unregisterApp"进行反注册以取消服务

   

### 		1.2 公共dbus服务接口列表

#### 1.2.1 version

##### 方法描述

> 当前接口的版本，因为 LLM 的更新速度非常快，可能不断有新的接口
>
> 因此，访问应用程序需要及时更新，或者通过判断版本与不同版本的 uos ai 兼容
>
> 此方法在 v1.0 中添加

##### 请求参数

> void

##### 返回类型

> String

##### 返回示例

```
"v1.1"
```



#### 1.2.2 registerApp

##### 方法描述

> 注册 dbus 服务，获取专属路径
>
> 此方法在 v1.1 中添加

##### 请求参数

> void

##### 返回类型

> String

##### 返回示例

```
"/com/deepin/copilot/app/deepin-mail"
```



#### 1.2.3 unregisterApp

##### 方法描述

> 反注册，删除独占的 dbus 服务
>
> 此方法在 v1.1 中添加

##### 请求参数

> void

##### 返回类型

> void



#### 1.2.4 queryUserExpState

##### 方法描述

> 检查是否同意用户协议
>
> 此方法在 v1.0 中添加

##### 请求参数

> void

##### 返回类型

> bool

##### 返回示例

```
false
```



#### 1.2.5 userExpStateChanged

##### 信号描述

> 信号，同意用户协议状态变化
>
> 此方法在 v1.0 中添加

##### 信号参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| state      | bool         | 表示最新用户协议状态 |



#### 1.2.6 launchChatPage

##### 方法描述

> 激活聊天对话框。
> 此方法在 v1.1 中添加

##### 请求参数

> | 字段  | 类型 | 说明                                                        |
> | ----- | ---- | ----------------------------------------------------------- |
> | index | int  | 0是文本聊天(默认），1是语音聊天；如果是已打开状态会执行切换 |

##### 返回类型

> void



#### 1.2.7 launchLLMUiPage

##### 方法描述

> 弹出 LLM 配置对话框。
> 此方法在 v1.0 中添加

##### 请求参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| openAddAccountDialog           | bool         | 同时弹出添加模型对话框 |

##### 返回类型

> void



#### 1.2.8 cachedFunctions

##### 方法描述

> 被调用程序要执行的功能列表
> 该方法需要在应用程序识别 --functioncall 参数后调用，以获取需要执行的任务
> 此函数在 v1.1 中添加

##### 请求参数

> void

##### 返回类型

> String JSON

##### 返回说明

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| functions           | array         | 函数数组 |
| functions.name      | string        | 函数名称 |
| functions.arguments | string object | 函数参数 |

##### 返回示例

```
{
	"functions": [{
		"name": "sendmail",
		"arguments": "{ \"to\": \"张三<zhangsan@uniontech.com>\",\"subject\": \"请假申请\"}"
	}]
 }
```



### 		1.3 专属dbus服务接口列表

#### 1.3.1 queryLLMAccountList

##### 方法描述

> 查询模型账号列表
> 此函数在 v1.0 中添加

##### 请求参数

> void

##### 返回类型

> String JSON

##### 返回说明

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| id           | string         | 模型ID号 |
| displayname      | string        | 模型自定义显示名称 |
| model | int | 模型枚举类型 |
| llmname | string | 模型名称 |

##### 模型枚举

```
    CHATGPT_3_5          = 0,   // GPT3.5
    CHATGPT_3_5_16K      = 1,   // GPT3.5 16k
    CHATGPT_4            = 2,   // GPT4
    CHATGPT_4_32K        = 3,   // GPT4 32k
    SPARKDESK            = 10,  // 迅飞星火1.5
    SPARKDESK_2          = 11,  // 迅飞星火2.0
    SPARKDESK_3          = 12,  // 迅飞星火3.0
    WXQF_ERNIE_Bot       = 20,  // 百度文心千帆ERNIE_Bot
    WXQF_ERNIE_Bot_turbo = 21,  // 百度文心千帆ERNIE_Bot_turbo
    WXQF_ERNIE_Bot_4     = 22,  // 百度文心千帆ERNIE_Bot_4
    GPT360_S2_V9         = 40,  // 360GPT
    ChatZHIPUGLM_PRO     = 50,  // 智谱AIPRO
    ChatZHIPUGLM_STD     = 51,  // 智谱AISTD
    ChatZHIPUGLM_LITE    = 52,  // 智谱AIpLITE
```

##### 返回示例

```
    [{\"id\":\"123456789\",\"displayname\":\"我的模型\",\"model":12,"llmname":"讯飞星火"}]
```


#### 1.3.2 currentLLMAccountId

##### 方法描述

> 当前模型账号ID
>
> 此方法在 v1.0 中添加

##### 请求参数

> void

##### 返回类型

> String

##### 返回示例

```
"12"
```



#### 1.3.3 setCurrentLLMAccountId

##### 方法描述

> 设置当前模型账号ID
>
> 此方法在 v1.0 中添加

##### 请求参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| id           | string         | 模型账号ID |

##### 返回类型

> bool

##### 返回示例

```
true
```



#### 1.3.4 requestChatText

##### 方法描述

> 聊天文本请求，如果开启了流式数据返回，需要连接返回值[0]的管道名称，接收流式数据，整个会话结束后，监听返回值[1]的信号，如果遇到错误，会收到error信号
>
> 此方法在 v1.0 中添加

##### 请求参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| llmId           | string         | 模型ID |
| conversation           | string         | 聊天内容聊天内容，如果只是一个文本字符串为单轮聊天会话，如果需要带上下文历史聊天会话，需要传入JsonArray数组字符串，格式如下|
| temperature           | double         | 较高的数值会使输出更加随机，而较低的数值会使其更加集中和确定，范围 (0, 1.0]，不能为0 |
| stream           | bool         | 是否以流式接口的形式返回数据，开启后可以通过本地管道通信持续获取数据 |

###### conversation

```
[
       {"role": "user", "content": "你好"},
       {"role": "assistant", "content": "我是人工智能助手"},
       {"role": "user", "content": "你叫什么名字"},
       {"role": "assistant", "content": "我叫chatGLM"},
       {"role":"user", "content":"你都可以做些什么事"},
] 
```

##### 返回类型

> stringlist
>[0]此次会话ID，如果开启了流式数据返回，此ID同时也是本地流数据通信管道名称
>[1]聊天请求结束信号名称



#### 1.3.5 cancelRequestTask

##### 方法描述

> 取消请求任务
>
> 此方法在 v1.0 中添加

##### 请求参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| id           | string         | 接口返回的ID号 |

##### 返回类型

> void


#### 1.3.6 error

##### 信号描述

> 信号，错误产生发出，应用根据需要提示相关信息
>
> 此方法在 v1.0 中添加

##### 信号参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| id      | string         | 接口返回的ID号 |
| code      | int         | 错误类型，具体内容见下方 |
| errorString      | string       | 错误消息，已对部分上游的错误进行了中文翻译 |

###### code

```
    NoError = 0,
    // network layer errors [relating to the destination server] (1-99):
    ConnectionRefusedError = 1,
    RemoteHostClosedError,
    HostNotFoundError,
    TimeoutError,
    OperationCanceledError,
    SslHandshakeFailedError,
    TemporaryNetworkFailureError,
    NetworkSessionFailedError,
    BackgroundRequestNotAllowedError,
    TooManyRedirectsError,
    InsecureRedirectError,
    UnknownNetworkError = 99,
    // proxy errors (101-199):
    ProxyConnectionRefusedError = 101,
    ProxyConnectionClosedError,
    ProxyNotFoundError,
    ProxyTimeoutError,
    ProxyAuthenticationRequiredError,
    UnknownProxyError = 199,
    // content errors (201-299):
    ContentAccessDenied = 201,
    ContentOperationNotPermittedError,
    ContentNotFoundError,
    AuthenticationRequiredError,
    ContentReSendError,
    ContentConflictError,
    ContentGoneError,
    UnknownContentError = 299,
    // protocol errors
    ProtocolUnknownError = 301,
    ProtocolInvalidOperationError,
    ProtocolFailure = 399,
    // Server side errors (401-499)
    InternalServerError = 401,
    OperationNotImplementedError,
    ServiceUnavailableError,
    UnknownServerError = 499,
    SocketAccessError = 600,
    SocketResourceError,
    SocketTimeoutError,
    DatagramTooLargeError,
    NetworkError,
    AddressInUseError,
    SocketAddressNotAvailableError,
    UnsupportedSocketOperationError,
    UnfinishedSocketOperationError,
    ProxyConnectionTimeoutError,
    ProxyProtocolError,
    OperationError,
    SslInternalError,
    SslInvalidUserDataError,
    TemporaryError = 699,
    SenSitiveInfoError = 8000,
    ServerRateLimitError = 8001,
    FREEACCOUNTEXPIRED = 9000,
    FREEACCOUNTUSAGELIMIT = 9001,
```



#### 1.3.7 chatTextReceived

##### 信号描述

> 信号，聊天文本接收
>
> 此方法在 v1.0 中添加

##### 信号参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| id      | string         | 接口返回的ID号 |
| chatText      | string         | 聊天文本内容 |



#### 1.3.8 llmAccountLstChanged

##### 信号描述

> 信号，模型账号列表变更
>
> 此方法在 v1.0 中添加

##### 信号参数

| 字段                | 类型          | 说明     |
| ------------------- | ------------- | -------- |
| currentAccountId      | string         | 当前模型ID |
| accountLst | string         | 模型账号列表，Json数组字符串JsonArray，同queryLLMAccountList返回 |



## 2. 加入AI小助手生态

### 2.1 功能接入	

​    应用需要注册Functioncall服务，安装到uos-ai助手指定目录下，以便助手判断并调用应用相关功能。安装流程如下：

#### 2.1.1 创建配置文件

应用创建json文件，字段说明如下 :

| 字段                                          | 类型   | 必填 | 说明                                                         |
| --------------------------------------------- | ------ | ---- | ------------------------------------------------------------ |
| appid                                         | string | 是   | dbus连接的appid，一般同进程名称                              |
| exec                                          | string | 是   | 二进制或脚本所在绝对路径                                     |
| functions                                     | array  | 是   | 支持的功能列表，因为模型容量限制，当前版本不能超过10个，多的会被丢弃 |
| functions.name                                | string | 是   | 函数名，需要满足[a-zA-Z0-9_-]{1,32} 格式                     |
| functions.description                         | string | 是   | 函数的描述，不能超过30个字符                                 |
| functions.parameters                          | object | 是   | 函数请求参数，包含多个object                                 |
| functions.parameters.type                     | string | 是   | 参数的类型，可以嵌套，最外层是"object"类型                   |
| functions.parameters.required                 | array  | 是   | 要求必须返回的参数数组                                       |
| functions.parameters.properties               | object | 是   | "object"类型的参数的属性，内部包含多个参数                   |
| functions.parameters.properties...description | string | 否   | 返回参数的描述，不能超过30个字符                             |
| functions.parameters.properties...type        | string | 是   | 返回参数的类型                                               |
| functions.parameters.properties...enum        | array  | 否   | 限制返回值结果是数组内某个值                                 |
| version                                       | string | 是   | 当前配置文件的版本号控制，同助手api版本                      |

##### 内容实例

```json
{
    "appid": "deepin-mail",
	"exec": "/usr/bin/deepin-mail",
	"functions": [
		{
			"name": "sendMail",
			"description": "发送邮件",
			"parameters": {
				"type": "object",
				"properties": {
					"subject": {
						"type": "string",
						"description": "邮件主题"
					},
					"content": {
						"type": "string",
						"description": "邮件正文"
					},
					"to": {
						"type": "string",
						"description": "收件人"
					},
					"cc": {
						"type": "string",
						"description": "抄送人"
					},
					"bcc": {
						"type": "string",
						"description": "密送人"
					}
				},
				"required": ["subject","content"]
			}
		},
        {
			"name": "switchMode",
			"description": "切换界面模式",
			"parameters": {
				"type": "object",
				"properties": {
					"mode": {
						"type": "string",
						"description": "Fashion时尚模式，Efficent高效模式",
						"enum": ["Fashion", "Efficent"]
					}
				},
				"required": ["mode"]
			}
		}
    ]
    "version": "1.1"
}
```

#### 2.1.2 安装配置文件

##### 文件命名

```
文件名可以以模块名称，保证不同名即可，如org.deepin.mail.json
```

##### 安装路径

```
/usr/lib/uos-ai-assistant/functions/
```

### 2.2 功能执行

##### 场景触发

  如果触发某应用的功能，uos-ai助手会缓存本次的方法名称和参数，执行配置文件中"exec"属性的值，唤起对应的应用，增加参数--functioncall，如：

```
/usr/bin/deepin-mail --functioncall
```

##### 任务执行

 被唤起的应用判断参数中包含"--functioncall"，可以向uos-ai助手请求需要执行的方法和参数，获取方法如"1.2.8"章节，得到的参数说明如下：

| 字段      | 类型   | 说明                           |
| --------- | ------ | ------------------------------ |
| name      | string | 本次应执行的应用注册的方法名称 |
| arguments | object | 本次应执行的应用注册的方法参数 |

注意：如果应用是唯一实例应用并为已运行状态，后启动的实例应按需将指令自行发送给当前正在运行的实例或者通知其去调用"cachedFunctions"，防止遗漏任务。

##### 返回示例

```json
{
	"functions": {
		"name": "get_current_weather",
		"arguments": "{ \"location\": \"Boston\"}"
	}
}
```

