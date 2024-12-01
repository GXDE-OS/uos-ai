// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UNIVERSALIMAGE_H
#define UNIVERSALIMAGE_H

#include "uosai_global.h"
#include "aiimages.h"

namespace uos_ai {

class UniversalImage : public AIImages
{
public:
    UniversalImage(const QString &url, const AccountProxy &account);
    QString rootUrlPath() const override;
private:
    QString rootUrl;
};

}


#endif // UNIVERSALIMAGE_H
