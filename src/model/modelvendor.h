#ifndef MODELVENDOR_H
#define MODELVENDOR_H

#include "modelinfo.h"

#include <QSharedPointer>
#include <QObject>

namespace uos_ai {
class ModelProvider;
class AbstractModel;
class ModelHubProvider;

class ModelVendor : public QObject
{
    Q_OBJECT
public:
    static ModelVendor *instance();

    QList<ModelAccountPtr> queryModels(const QVariantHash &condition) const;
    ModelAccountPtr getModel(const QString &id) const;
    QSharedPointer<AbstractModel> createModel(const ModelAccountPtr &acc) const;
    QSharedPointer<ModelProvider> provider(const QString &id) const;
    void removeProvider(const QString &id);
    void removeModel(const QString &id);
    static QString networkType(const ModelAccountPtr &);
    static bool isValid(const ModelAccountPtr &);
    static bool isUosProvider(const ModelAccountPtr &);
    static bool isUosProvider(const QString &provider);
signals:
    void modelChanged();
public slots:
    void refresh();
private:
    void refreshLocal() const;
    explicit ModelVendor(QObject *parent = nullptr);
private:
    QMap<QString, ModelAccountPtr> m_accounts;
    mutable QMap<QString, ModelAccountPtr> m_local;
#ifdef ENABLE_LOCAL_MODEL
    QSharedPointer<ModelHubProvider> m_modelhub;
#endif
};


}

#endif // MODELVENDOR_H
