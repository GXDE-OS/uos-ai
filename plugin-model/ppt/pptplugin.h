#ifndef PPTPLUGIN_H
#define PPTPLUGIN_H

#include <llmplugin.h>

#include <QObject>
#include <QVector>

namespace uos_ai {

class PPTPlugin : public QObject, public LLMPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LLMPlugin_Meta_IID FILE "plugin.json")
    Q_INTERFACES(uos_ai::LLMPlugin)
public:
    explicit PPTPlugin(QObject *parent = nullptr);
    QStringList modelList() const override;
    QStringList roles(const QString &model) const override;
    QVariant queryInfo(const QString &query, const QString &id) override;
    LLMModel *createModel(const QString &name) override;
    void loadFAQ();
private:
    QVector<QJsonObject> m_pptcreateFAQ;
};

}
#endif // PPTPLUGIN_H
