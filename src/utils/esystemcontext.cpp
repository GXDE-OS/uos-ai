#include "esystemcontext.h"

#include <DSysInfo>

#include <QProcess>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QDebug>

bool ESystemContext::isWayland()
{
    static bool hasChecked = false;
    static bool wayland = false;

    if (hasChecked)
        return wayland;

    auto environment = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = environment.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = environment.value(QStringLiteral("WAYLAND_DISPLAY"));

    if (XDG_SESSION_TYPE == QLatin1String("wayland") || WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        wayland = true;
    }

    hasChecked = true;
    return wayland;
}

bool ESystemContext::needToDoneCurrentForOpenGL()
{
    if (!isWayland())
        return false;

    DCORE_USE_NAMESPACE

    static bool hasChecked = false;
    static bool isNeeded = false;

    if (hasChecked)
        return isNeeded;

    hasChecked = true;

    int minorVersion = DSysInfo::minorVersion().toInt();

    //超过1050默认安装驱动
    if (minorVersion > 1050) {
        return false;
    }

    int buildVersion = DSysInfo::buildVersion().toInt();
    //1050版本且构建版本大于等于107默认安装驱动
    if (minorVersion == 1050 && buildVersion >= 107) {
        return false;
    }

    //其它情况没有安装驱动
    isNeeded = true;
    return true;
}

void ESystemContext::openGLContextDoneCurrent()
{
    if (!needToDoneCurrentForOpenGL())
        return;

    if (QOpenGLContext *context = QOpenGLContext::currentContext()) {
        context->doneCurrent();
    }
}

bool ESystemContext::isSupportOpenGL()
{
    QOpenGLContext ctx;
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    ctx.setFormat(fmt);
    if (!ctx.create())
        return false;
    if (!ctx.isValid())
        return false;

    return (ctx.format().renderableType() == QSurfaceFormat::OpenGL);
}

bool ESystemContext::isSupportOpenGLES()
{
    QOpenGLContext ctx;
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGLES);
    ctx.setFormat(fmt);
    if (!ctx.create())
        return false;
    if (!ctx.isValid())
        return false;

    return (ctx.format().renderableType() == QSurfaceFormat::OpenGLES);
}

void ESystemContext::configOpenGL()
{
    if (!isSupportOpenGL()) {
        qWarning() << "OpenGL is not supported!!!";

        if (isSupportOpenGLES()) {
            QSurfaceFormat format;
            format.setDepthBufferSize(24);
            format.setRenderableType(QSurfaceFormat::OpenGLES);
            format.setDefaultFormat(format);
        } else {
            qWarning() << "OpenGLES is not supported!!!";
        }
    }
}
