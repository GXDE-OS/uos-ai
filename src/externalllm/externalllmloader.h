// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERNALLLMLOADER_H
#define EXTERNALLLMLOADER_H

#include <QObject>

namespace uos_ai {
class LLMPlugin;
class ExternalLLMLoaderPrivate;
class ExternalLLMLoader : public QObject
{
    Q_OBJECT
    friend class ExternalLLMLoaderPrivate;
public:
    explicit ExternalLLMLoader(QObject *parent = nullptr);
    ~ExternalLLMLoader();
    void setPaths(const QStringList &paths);
    void readPlugins();
    QList<QSharedPointer<LLMPlugin>> plugins() const;
signals:

public slots:
private:
    ExternalLLMLoaderPrivate *d;
};

}

#endif // EXTERNALLLMLOADER_H
