// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#pragma once
#include "qwayland-treeland-dde-shell-v1.h"

#include <QGuiApplication>
#include <QObject>
#include <QWaylandClientExtension>
#include <QMainWindow>

#define TREELANDDDESHELLMANAGERV1VERSION 1

class TreelandDDEShellManageV1
    : public QWaylandClientExtensionTemplate<TreelandDDEShellManageV1>
    , public QtWayland::treeland_dde_shell_manager_v1
{
    Q_OBJECT
public:
    TreelandDDEShellManageV1();
    ~TreelandDDEShellManageV1() = default;

    void instantiate();
};

class TreelandDDEActiveV1
    : public QObject
    , public QtWayland::treeland_dde_active_v1
{
    Q_OBJECT
public:
    TreelandDDEActiveV1(struct ::treeland_dde_active_v1 *id);
    ~TreelandDDEActiveV1();

protected:
    void treeland_dde_active_v1_active_in(uint32_t reason) override;
    void treeland_dde_active_v1_active_out(uint32_t reason) override;
    void treeland_dde_active_v1_start_drag() override;
    void treeland_dde_active_v1_drop() override;

Q_SIGNALS:
    void mousePress();
    void mouseRelease();
};
