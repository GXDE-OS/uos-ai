#ifndef CONFIGTABLE_H
#define CONFIGTABLE_H
#include <tables/dbbase.h>

#include <QStringList>
#include <QDateTime>
#include <QVariant>
#include <QSharedDataPointer>

class ConfigObject;
class ConfigTable: public DbBase
{
public:
    enum ConfigType {
        CopilotSwitch   = 1,    // 小助手总开关
        CopilotVersion  = 2,    // 小助手版本
        CopilotTheme    = 3,    // 主题(Light、Mirage、Dark)
        CopilotUserExp  = 4,    // 小助手用户体验计划状态
        CopilotLocalSpeech = 5, // 本地语音模型
    };

    ConfigTable();
    ConfigTable(const ConfigTable  &other);
    ConfigTable(const ConfigObject &object);

    ~ConfigTable();

    ConfigTable  &operator=(const ConfigTable  &other);

    const ConfigObject *modelData() const;

    int id();

    QString name() const;
    void setName(const QString &pass);

    int type() const;
    void setType(const int &type);

    QString desc() const;
    void setDesc(const QString &desc);

    QString value() const;
    void setValue(const QString &value);

    virtual bool save() override ;
    virtual bool update() override ;
    virtual bool remove() override ;

    static ConfigTable  create();

    static ConfigTable  create(int id, const QString &name, int type, const QString &desc, const QString &value);

    static ConfigTable  get(int type);

    static QList<ConfigTable> getAll();

    static int count();

private:
    QSharedDataPointer<ConfigObject> d;
};

#endif // CONFIGTABLE_H
