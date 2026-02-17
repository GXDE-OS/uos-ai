#include "aibarconfig.h"
#include "dconfigmanager.h"

#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QGuiApplication>
#include <QDir>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIBar)

namespace uos_ai {
class AiBarConfigGlobal : public AiBarConfig {};
Q_GLOBAL_STATIC(AiBarConfigGlobal, configerGlobal)
}

using namespace uos_ai;

AiBarConfig *AiBarConfig::instance()
{
    return configerGlobal;
}

AiBarConfig::AiBarConfig(QObject *parent)
    : QObject{parent}
{
}

bool AiBarConfig::getEnableFileDrag() const
{
    bool enableFileDrag = DConfigManager::instance()->value(AIBAR_GROUP, AIBAR_ENABLEFILEDRAG, true).toBool();
    qCDebug(logAIBar) << "File drag feature status:" << enableFileDrag;
    return enableFileDrag;
}

void AiBarConfig::onFileChanged(const QString &path)
{
    qCInfo(logAIBar) << "Configuration file changed:" << path;
}

void AiBarConfig::onLoadConfig()
{
}

void AiBarConfig::init()
{
}
