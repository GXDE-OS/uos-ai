#ifndef AIBARCONFIG_H
#define AIBARCONFIG_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QSettings>

#define ConfigerIns()  uos_ai::AiBarConfig::instance()

namespace uos_ai {

class AiBarConfig : public QObject
{
    Q_OBJECT
public:
    static AiBarConfig *instance();
    explicit AiBarConfig(QObject *parent = nullptr);
    bool getEnableFileDrag() const;

signals:
protected slots:
    void onFileChanged(const QString &path);
    void onLoadConfig();
private:
    void init();
private:
};

}
#endif // AIBARCONFIG_H
