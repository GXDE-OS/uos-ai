// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EXTERALLLMLOADER_H
#define EXTERALLLMLOADER_H

#include <QObject>

namespace uos_ai {
class LLMPlugin;
class ExteralLLMLoaderPrivate;
class ExteralLLMLoader : public QObject
{
    Q_OBJECT
    friend class ExteralLLMLoaderPrivate;
public:
    explicit ExteralLLMLoader(QObject *parent = nullptr);
    ~ExteralLLMLoader();
    void setPaths(const QStringList &paths);
    void readPlugins();
    QList<QSharedPointer<LLMPlugin>> plugins() const;
signals:

public slots:
private:
    ExteralLLMLoaderPrivate *d;
};

}

#endif // EXTERALLLMLOADER_H
