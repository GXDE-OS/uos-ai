#ifndef WXQFTEXT2IMAGE_H
#define WXQFTEXT2IMAGE_H

#include "wxqfnetwork.h"

class WXQFText2Image : public WXQFNetWork
{
public:
    explicit WXQFText2Image(const AccountProxy &account);
    QPair<int, QString> create(const QString &prompt, QList<QByteArray> &imageData, int number);
};

#endif // WXQFTEXT2IMAGE_H
