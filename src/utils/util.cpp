// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <gio/gio.h>

#include "util.h"

#include <DDesktopServices>
#include <DWidget>
#include <DtkGuis>
#include <DSysInfo>
#include <DTextEncoding>

#include <QVariant>
#include <QProcessEnvironment>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QMimeDatabase>
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QPixmap>
#include <QPainter>
#include <QFileInfo>
#include <QImageReader>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logUtils)

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
UOSAI_USE_NAMESPACE

Util::Util()
{

}

bool Util::isWayland()
{
    static int wayland = -1;
    if (wayland < 0) {
        auto e = QProcessEnvironment::systemEnvironment();
        QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
        QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));
        if (XDG_SESSION_TYPE == QLatin1String("wayland") || WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
            wayland = 1;
        } else {
            wayland = 0;
        }
        qCInfo(logUtils) << "Check is wayland:" << wayland;
    }

    return wayland == 1;
}

QString Util::getExeNameByPid(int pid)
{
    QString exeName;
    QString linkpath = QString("/proc/%1/exe").arg(pid);
    QString path = QFile::symLinkTarget(linkpath);
    auto args = path.split("/");
    exeName = args.length() == 0 ? "" : args[args.length() -1];
    if (exeName.length() == 0) {
        //如：/usr/bin/xxxx -e xx
        QString cmdline;
        QFile fileCmd(QString("/proc/%1/cmdline").arg(pid));
        if (fileCmd.open(QFile::ReadOnly)) {
            cmdline = QString(fileCmd.readLine());
            fileCmd.close();
        }
        QStringList list = cmdline.split("/");
        QString program = list.last();
        list = program.split(" ");
        exeName = list.first();
    }
    return exeName;
}

void Util::playSystemSound_SSE_Error()
{
    DDesktopServices::playSystemSoundEffect(
                DDesktopServices::SystemSoundEffect::SSE_Error);
}

bool Util::isGPTEnable()
{
    qCDebug(logUtils) << "Check productType:" << DSysInfo::productType()
             << "language:" << QLocale::system().language() << QLocale::system().script();
    if (isCommunity())
        return true;

    return !checkLanguage();
}

bool Util::isGPTSeries(LLMChatModel m)
{
    static QSet<LLMChatModel> gptSeries = {
        CHATGPT_3_5, CHATGPT_3_5_16K,
        CHATGPT_4, CHATGPT_4_32K,
        GEMINI_1_5_FLASH, GEMINI_1_5_PRO, OPENROUTER_MODEL
    };

    return gptSeries.contains(m);
}

bool Util::isCommunity()
{
    return DSysInfo::productType() == DSysInfo::Deepin;
}

bool Util::checkLanguage()
{
    static const QList<QLocale> supportLanguage{{QLocale::Tibetan}, {QLocale::Uighur}, {QLocale::Chinese}};
    QLocale locale = QLocale::system();
    return supportLanguage.contains(locale.language());
}

QString Util::generateAssistantUuid(QString name)
{
    QString formatName = name.replace(" ", "-").toLower().trimmed();
    return  formatName;
}

bool Util::isAIDaemonExist()
{
    QDir dir("/usr/bin");
    if (!dir.exists()) {
        return false;
    }

    QFileInfo fileInfo(dir.filePath("deepin-ai-daemon"));
    return fileInfo.exists() && fileInfo.isFile();
}

QString Util::textEncodingTransferUTF8(const std::string &content)
{
    if (content.empty())
        return "";

    QByteArray data = QByteArray::fromStdString(content);
    QString codeFormat = Dtk::Core::DTextEncoding::detectTextEncoding(data);
    if (codeFormat.toLower() == QString("utf-8"))
        return QString::fromStdString(content);

    QByteArray out;
    if (Dtk::Core::DTextEncoding::convertTextEncoding(data, out, "utf-8", codeFormat.toUtf8())) {
        return QString::fromUtf8(out);
    } else {
        qCWarning(logUtils) << "Failed to convert encoding from" << codeFormat << "to UTF-8";
        return "";
    }
}

bool Util::isValidDocContent(const std::string &content)
{
    QByteArray data = QByteArray::fromStdString(content);

    QMimeDatabase mimeDB;
    QMimeType mimeType = mimeDB.mimeTypeForData(data);
    if (!mimeType.isValid())
        return false;

    if (mimeType.name().contains("text"))
        return true;

    return false;
}

bool Util::openFileFromPath(const QString &path)
{
    QFileInfo fileInfo(path);

    bool openOK = false;

    if (fileInfo.exists()) {
        openOK = QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    } else {
        qCWarning(logUtils) << __FUNCTION__
                   << " file can't find:" << path;
    }

    return openOK;
}

bool Util::launchUosBrowser(QString url) {
    bool ret = false;
    static QString mimetype = "x-scheme-handler/http";
    GList *list = g_app_info_get_all_for_type(mimetype.toStdString().c_str());
    // 遍历列表
    for (GList *l = list; l != nullptr; l = l->next) {
        // 注意：不需要手动释放 GAppInfo 对象，因为它们是由 g_app_info_get_all_for_type 管理的
        GAppInfo *appInfo = G_APP_INFO(l->data);
        const char *APP_PATH = g_app_info_get_executable(appInfo);
        qCInfo(logUtils) << "Find browser:" << APP_PATH;
        // /usr/bin/browser
        if (QString(APP_PATH).endsWith("/browser")) {
            GList *g_files = nullptr;
            GFile *f = g_file_new_for_uri(url.toStdString().c_str());
            g_files = g_list_append(g_files, f);
            GError *gError = nullptr;
            qCInfo(logUtils) << "Browser:" << url;
            gboolean ok = g_app_info_launch(appInfo, g_files, nullptr, &gError);
            if (gError) {
                qCWarning(logUtils) << "Error when trying to open desktop file with gio: " << gError->message;
                g_error_free(gError);
            }
            if (!ok) {
                qCWarning(logUtils) << "Failed to open desktop file with gio: g_app_info_launch returns false";
            } else {
                ret = true;
            }
            g_list_free_full(g_files, g_object_unref);
            break;
        }

        // 如果是玲珑环境，浏览器被玲珑接管，参数需要直接跟网址（如果玲珑环境安装了多个浏览器，会使用新安装的那个）
        // /usr/bin/ll-cli
        if (QString(APP_PATH).endsWith("/ll-cli")) {
            GList *g_files = nullptr;
            GFile *f = g_file_new_for_uri(url.toStdString().c_str());
            g_files = g_list_append(g_files, f);
            GError *gError = nullptr;
            qCInfo(logUtils) << QString("ll-cli %1").arg(QString("run org.deepin.browser -- browser ") + url);
            gboolean ok = g_app_info_launch(appInfo, g_files, nullptr, &gError);
            if (gError) {
                qCWarning(logUtils) << "Error when trying to open desktop file with gio: " << gError->message;
                g_error_free(gError);
            }
            if (!ok) {
                qCWarning(logUtils) << "Failed to open desktop file with gio: g_app_info_launch returns false";
            } else {
                ret = true;
            }
            g_list_free_full(g_files, g_object_unref);
            break;
        }
    }
    // 释放 GList 列表
    g_list_free_full(list, g_object_unref);

    return ret;
}

bool Util::launchDefaultBrowser(QString url) {
    static QString mimetype = "x-scheme-handler/http";
    GAppInfo *defaultApp = g_app_info_get_default_for_type(mimetype.toLocal8Bit().constData(), FALSE);
    if (!defaultApp) {
        return false;
    }

    bool ret = false;
    GList *g_files = nullptr;
    GFile *f = g_file_new_for_uri(url.toStdString().c_str());
    g_files = g_list_append(g_files, f);
    GError *gError = nullptr;
    qCInfo(logUtils) << "Browser:" << url;
    gboolean ok = g_app_info_launch(defaultApp, g_files, nullptr, &gError);
    if (gError) {
        qCWarning(logUtils) << "Error when trying to open desktop file with gio: " << gError->message;
        g_error_free(gError);
    }
    if (!ok) {
        qCWarning(logUtils) << "Failed to open desktop file with gio: g_app_info_launch returns false";
    } else {
        ret = true;
    }
    g_list_free_full(g_files, g_object_unref);
    g_object_unref(defaultApp);

    return ret;
}

QString Util::imageData2TmpFile(const QString &tmpDir, const QString &imageData)
{
    QString dataHash = QCryptographicHash::hash(imageData.toUtf8(), QCryptographicHash::Md5).toHex(); //TODO 可能存在hash冲突
    QString imageFileName = QString("%1/%2.jpg")
                            .arg(tmpDir)
                            .arg(dataHash);
    QFile file(imageFileName);
    if (file.exists())
        return imageFileName;

    if (file.open(QIODevice::WriteOnly)) {
        file.write(QByteArray::fromBase64(imageData.toUtf8()));
        file.close();
    }

    return imageFileName;
}

QPixmap Util::loadSvgPixmap(const QString &filePath, const QSize &size)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qCWarning(logUtils) << "SVG file does not exist:" << filePath;
        return QPixmap();
    }

    auto ratio = qApp->devicePixelRatio();

    QPixmap originalPixmap(filePath);
    if (originalPixmap.isNull()) {
        qCWarning(logUtils) << "Failed to load SVG file:" << filePath;
        return QPixmap();
    }

    // 检查 QImageReader 是否支持 SVG 格式
    QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
    bool supportsSvg = supportedFormats.contains("svg") || supportedFormats.contains("SVG");

    if (supportsSvg) {
        QImageReader reader(filePath);
        reader.setFormat("svg");
        reader.setScaledSize(size * ratio);
        reader.setQuality(100);
        QImage image = reader.read();
        if (!image.isNull()) {
            if (image.size() != size * ratio) {
                image = image.scaled(size * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            QPixmap pixmap = QPixmap::fromImage(image);
            pixmap.setDevicePixelRatio(ratio);
            return pixmap;
        } else {
            qCWarning(logUtils) << "Failed to read SVG image with QImageReader:" << filePath << "Error:" << reader.errorString();
        }
    }


    QPixmap pmap(size * ratio);
    pmap.fill(Qt::transparent);

    {
        QPainter painter(&pmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawPixmap(QRect(QPoint(0, 0), size * ratio), originalPixmap);
    }

    pmap.setDevicePixelRatio(ratio);
    return pmap;
}

QString Util::splitLocaleName(const QString &locale)
{
    QString ret;

    // 去掉_后的本地语言标识
    QStringList localeList = locale.split("_");
    if (localeList.size() == 2 && !localeList.first().isEmpty())
        ret = localeList.first();

    return ret;
}

