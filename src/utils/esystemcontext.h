#ifndef ESYSTEMCONTEXT_H
#define ESYSTEMCONTEXT_H

/**
 * @brief 获取系统各种环境变量
 */
class ESystemContext
{
public:
    static bool isWayland();

    /**
     * @brief 由于hw线klu和pangv下更新了显卡驱动，不需要手动延时释放，这里对高版本进行区分
     */
    static bool needToDoneCurrentForOpenGL();

    static bool isSupportOpenGL();

    static bool isSupportOpenGLES();

    static void configOpenGL();

    static void openGLContextDoneCurrent();

};

#endif // ESYSTEMCONTEXT_H
