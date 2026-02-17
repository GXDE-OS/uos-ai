#ifndef VOICEANIMATIONWIDGET_H
#define VOICEANIMATIONWIDGET_H
#include <DWidgetUtil>
#include <DPalette>
#include <DWidget>
#include <QTimer>
#include <DLabel>
#include <DPushButton>
#include <DCommandLinkButton>
#include "uosai_global.h"

namespace uos_ai {
class VoiceAnimationWidget : public DTK_WIDGET_NAMESPACE::DWidget
{
    Q_OBJECT

public:

    VoiceAnimationWidget(QWidget *parent = nullptr);

    void updateMaxLevel(int level);

    void startPaint();

    void stopPaint();

private:

    void paintEvent(QPaintEvent* event);

    void resizeEvent(QResizeEvent* event);

    double getAudioLineY(double x,int type);



private:
    double          m_offset1,m_offset2,m_offset3;
    double          m_currentLevel;
    double          m_maxLevel;
    int             m_receiveAudioSum;
    QTimer*         m_timer;
    bool            m_audioSpeed;
    double          m_audioWidth,m_audioHeight;
    bool            m_isPainting;
};
}

#endif // VOICEANIMATIONWIDGET_H
