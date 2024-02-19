#ifndef AIIMAGES_H
#define AIIMAGES_H

#include "ainetwork.h"

#include <QPixmap>

class AIImages : public AINetWork
{
public:
    explicit AIImages(const AccountProxy &account);

public:
    /**
     * @brief create
     * @param prompt 最大长度为 1000 个字符
     * @param n 要生成的图像数。必须介于 1 和 10 之间
     * @param size 生成图像的大小。必须是256x256、512x512或 之一1024x1024
     * @param format 生成的图像返回的格式。必须是 或url之一b64_json
     */
    QPair<int, QString> create(const QString &prompt, QList<QByteArray> &imageData, int n = 4, const QString &size = "512x512", const QString &format = "b64_json");

private:
    /**
     * @brief parserImages
     * @param data
     * @return
     */
    QList<QByteArray> parserImages(const QByteArray &data) const;
};

#endif // IMAGES_H
