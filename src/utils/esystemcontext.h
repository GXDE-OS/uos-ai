#ifndef ESYSTEMCONTEXT_H
#define ESYSTEMCONTEXT_H

#ifdef COMPILE_ON_V25
#include <DPlatformWindowHandle>
#include <QWindow>
#endif

/**
 * @brief 获取系统各种环境变量
 */
class ESystemContext
{
public:
    static bool isWayland();
    static bool isTreeland();

    /**
     * @brief 由于hw线klu和pangv下更新了显卡驱动，不需要手动延时释放，这里对高版本进行区分
     */
    static bool needToDoneCurrentForOpenGL();

    static bool isSupportOpenGL();

    static bool isSupportOpenGLES();

    static void configOpenGL();

    static void openGLContextDoneCurrent();

    template<typename T, typename... Args>
    static T *createWebWindow(Args... args) {
        T *win = nullptr;
#ifdef COMPILE_ON_V25
        //! dtk6中 DMainWindow和QWebEngineView一起使用会崩溃，
        //! 原因是MainWindow默认构造使用的是RasterSurface，而WebEngineView需要GLSurface
        //! 因此这些设置Window创建使用OpenGLSurface
        DTK_WIDGET_NAMESPACE::DPlatformWindowHandle::setWindowSurfaceType(QWindow::OpenGLSurface);
        win = new T(args...);
        DTK_WIDGET_NAMESPACE::DPlatformWindowHandle::setWindowSurfaceType(-1);
#else
        win = new T(args...);
#endif
        return win;
    }
};

#endif // ESYSTEMCONTEXT_H
