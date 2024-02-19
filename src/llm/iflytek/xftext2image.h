#ifndef XFTEXT2IMAGE_H
#define XFTEXT2IMAGE_H

#include "xfnetwork.h"
#include "xfconversation.h"

class XFText2Image: public XFNetWork
{
public:
    XFText2Image(const AccountProxy &account);
    QPair<int, QString> create(int model, XFConversation &conversation, int number);
};

#endif // XFTEXT2IMAGE_H
