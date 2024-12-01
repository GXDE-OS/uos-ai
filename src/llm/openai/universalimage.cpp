// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "universalimage.h"

UOSAI_USE_NAMESPACE

UniversalImage::UniversalImage(const QString &url, const AccountProxy &account)
    : AIImages(account)
    , rootUrl(url)
{

}

QString UniversalImage::rootUrlPath() const
{
    return rootUrl;
}
