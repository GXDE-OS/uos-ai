#ifndef FUNCTIONPARSER_H
#define FUNCTIONPARSER_H

#include <QObject>
#include <QString>
#include <QRegularExpression>

namespace uos_ai {

class ToolParser : public QObject
{
    Q_OBJECT
public:
    /**
     * 解析结果类型枚举
     */
    enum Type{ 
        Text = 0,       // 普通文本内容
        Function        // 工具调用内容
    };
    
    /**
     * 构造函数
     * @param parent 父对象指针
     */
    explicit ToolParser(QObject *parent = nullptr);

    /**
     * 向解析器输入数据块
     * 流式处理输入的数据，实时解析文本和工具调用
     * @param {const QString&} input - 要处理的数据块，可以是部分内容
     */
    void feed(const QString &input);

    /**
     * 完成解析过程，处理剩余缓冲区内容
     * 当没有更多输入时调用，确保缓冲区中的内容被正确处理
     */
    void finalize();

    /**
     * 获取解析结果
     * @returns {QList<QPair<int, QString>>} 解析结果列表
     *          QPair的第一个元素是Type枚举值，第二个元素是内容字符串
     */
    QList<QPair<int, QString>> getResults();

    /**
     * 将函数调用内容转换为JSON格式
     * 解析工具调用的字符串格式，提取函数名称和参数
     * @param {const QString&} func_content - 函数调用内容字符串
     * @param {QString&} name - 输出参数，解析出的函数名称
     * @param {QJsonObject&} args - 输出参数，解析出的函数参数JSON对象
     * @returns {bool} 转换是否成功，true表示成功解析，false表示解析失败
     */
    static bool toJson(const QString &func_content, QString &name, QJsonObject &args);

    /**
     * 恢复函数调用的原始格式
     * 将处理过的函数调用内容恢复为原始格式
     * @param {const QString&} content - 要恢复的内容
     * @returns {QString} 恢复后的函数调用字符串
     */
    static QString restoreFunction(const QString &content);

    /**
     * 向结果列表中添加解析结果
     * @param {Type} type - 内容类型（文本或工具调用）
     * @param {const QString&} content - 内容字符串
     */
    void pushResult(Type type, const QString &content);

private:
    /**
     * 检查并处理待处理的标签
     * 处理可能的工具调用开始标签
     */
    void checkPendingTag();

    /**
     * 处理函数调用内容
     * 解析完整的函数调用块，提取函数名称和参数
     */
    void processFunction();

    bool m_inFunction = false;              // 是否正在处理函数调用状态标志
    QString m_buffer;                       // 输入缓冲区，存储待处理的数据
    QString m_pendingTag;                   // 待处理的标签内容
    QList<QPair<int, QString>> m_results;   // 解析结果列表，存储所有解析出的内容
};

}
#endif // FUNCTIONPARSER_H
