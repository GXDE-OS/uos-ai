#ifndef TASDEF_H
#define TASDEF_H
#include <QString>
#include <QVector>
#include <QMap>
#include <QMetaType>
#include <QDateTime>

enum TextAuditEnum {
    None,       // NONE
    Auditing,   // Auditing
    Normal,     // 正常文本
    Spam,       // 含垃圾信息
    Ad,         // 广告
    Politics,   // 涉政
    Terrorism,  // 暴恐
    Abuse,      // 辱骂
    Porn,       // 色情
    Flood,      // 灌水
    Contraband, // 违禁
    Meaningless, // 无意义
    Harmful,    // 不良场景（支持拜金炫富、追星应援、负面情绪、负面诱导等检测场景）
    Customized, // 自定义（例如命中自定义关键词）
    NetError    // 网络错误
};

struct TextAuditResult {
    TextAuditEnum code;
    QString content;
    QString message;
};

struct TAPosition {
    int startPos;
    int endPos;
} ;

struct TADetails {
    TextAuditResult type;
    QString context;
    QVector<TAPosition> positions;
};


enum UosLogType {
    UserInput = 1,
    FailedRetry,
    TextToImageResult,
    AnwserRate
};

struct UosLogObject {
    UosLogType type;  // 日志类型
    QString app;
    QString content;
    QDateTime time;
    QString llm;
    QString ModelType;
    QString assistant;
    int t2iResult; // 0成功 1失败
} ;

struct UosRateLog {
    enum Rate{
        None = -1,
        Like = 1,
        Dislike = 2,
        Cancle = 3
    };

    UosLogType type;  // 日志类型
    QString question;
    QString answer;
    QString questionTime;
    QString answerTime;
    QString llm;
    QString app;
    QString modelType;
    QString assistantName;
    Rate likeOrNot = None;
};

struct UosFreeAccountActivity {
    QString buttonNameChina;  // 按钮名称中文
    QString buttonNameEnglish; // 按钮名称英文
    QString url; // 跳转的url地址
    int display; // 是否显示 0:隐藏，1:显示
    QString type; // 类型，根据类型判断哪个页面
    QDateTime startTime; // 开始时间
    QDateTime endTime; // 结束时间
};

struct UosFreeModelActivity {
    int type; // 模型类型：deepseek 81, 百度千帆 20
    bool inActivityPeriod; // 账号是否免费发放活动期间 true：该大模型免费账号发放中；false：该大模型免费账号活动结束
    QDateTime startTime; // 开始时间
    QDateTime endTime; // 结束时间
};

struct UosFreeAccount {
    int type; // 账号类型:普通账号 general，KOL账号 KOL
    QString appid; // 账号
    QString appkey; // 账号
    QString appsecret; // 账号
    int useLimit; // 最大使用限制
    bool hasUsed; // 已经使用次数
    int llmModel; // 大模型类型
    QString modelUrl; // 大模型接口地址
    QDateTime expTime; // 失效时间
    QDateTime startTime; // 开始时间
    QDateTime endTime; // 结束时间
};

#endif // TASDEF_H
