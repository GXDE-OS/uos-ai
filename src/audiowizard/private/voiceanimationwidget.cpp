#include "voiceanimationwidget.h"
#include <QtMath>
#include <QResizeEvent>
#include <QDebug>
#include <DFontSizeManager>
#include <QBrush>
#include <QDBusInterface>
#include <QPainterPath>
#include <QLoggingCategory>

#define PI 3.1415926

DWIDGET_USE_NAMESPACE
using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logAudioWizard)

VoiceAnimationWidget::VoiceAnimationWidget(QWidget *parent) : DWidget(parent)
  , m_timer(new QTimer)
{
    m_offset1=0;
    m_offset2=0;
    m_offset3=0;
    m_audioSpeed=true;
    m_maxLevel =10;
    connect(m_timer,&QTimer::timeout,[&](){
        if(m_currentLevel < m_maxLevel && m_audioSpeed ) {
            m_currentLevel++;
        }

        if(m_currentLevel >=m_maxLevel) {
            m_audioSpeed =false;
        }
        if(!m_audioSpeed) {
            if(m_currentLevel>(m_maxLevel-5)) {
                m_currentLevel--;
            }
            else {
                m_audioSpeed=true;
            }
        }
        update();
    });
    m_currentLevel=5;
}


void VoiceAnimationWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
}

void VoiceAnimationWidget::updateMaxLevel(int level)
{
    m_maxLevel = qMin(100, qMax(10, level));
}

void VoiceAnimationWidget::startPaint()
{
    qCDebug(logAudioWizard) << "Starting voice animation painting";
    m_isPainting = true;
    m_timer->start(15);
    show();
}

void VoiceAnimationWidget::stopPaint()
{
    qCDebug(logAudioWizard) << "Stopping voice animation painting";
    m_isPainting = false;
    m_timer->stop();
    m_currentLevel=5;
    m_maxLevel =10;
    m_offset1=0;
    m_offset2=0;
    m_offset3=0;
    update();
    hide();
}

void VoiceAnimationWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if(m_isPainting) {
        QPainter painter(this);
        double w = this->width();
        double h = this->height();
        painter.translate(w/2, h);//设置坐标轴原点
        painter.scale(1, -1);//设置坐标轴正负方向

        m_offset1+=PI/12;
        m_offset2+=PI/13;
        m_offset3+=PI/18;
        QPainterPath wave1,wave2,wave3; //波浪区域
        wave1.moveTo(-w/2, 0);//第一点坐标为;
        wave2.moveTo(-w/2, 0);
        wave3.moveTo(-w/2, 0);
        for(double x = -w/2; x <= w/2; x++)
        {
            double waveY1,waveY2, waveY3;
            waveY1=getAudioLineY(x, 1);
            wave1.lineTo(x, waveY1);   //从上一个绘制点画一条线到（x，waveY）；

            waveY2=getAudioLineY(x, 2);
            wave2.lineTo(x, waveY2);

            waveY3=getAudioLineY(x, 3);
            wave3.lineTo(x, waveY3);
        }
        wave1.lineTo(w/2, 0); //右下角，坐标（width, height），移动到右下角结束点,整体形成一个闭合路径
        wave2.lineTo(w/2, 0);
        wave3.lineTo(w/2, 0);

        QLinearGradient linear(QPointF(-w/2, 0), QPointF(w/2, 0));//设置渐变
        linear.setSpread(QGradient::PadSpread);
        linear.setColorAt(0, QColor(14, 136, 250, 160));
        linear.setColorAt(1, QColor(147, 247, 67, 160));

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QColor(0, 0, 0, 0));
        painter.setBrush(linear);
        painter.drawPath(wave1);      //绘制出图形
        painter.drawPath(wave2);
        painter.drawPath(wave3);
    }
}

double VoiceAnimationWidget::getAudioLineY(double x, int type)
{
    double y = 0;
    double w = this->width();
    double h = this->height();
    if(type == 1)
    {
        y = (m_currentLevel/100)*(h/4)*(cos(2*PI/w*x)+0.9);
        y = 0.7*(sin(4/w*PI*x+m_offset1)+2)*y;
    }
    else if(type == 2)
    {
        if(x>(-w/4) && x<w/2)
        {
            y = (m_currentLevel/100)*(h/5)*(cos((x-w/6)*(8*PI)/(3*w))+1);
            y = 0.9*(sin(4/w*PI*x-m_offset2)+1.5)*y;
        }
    }
    else if(type == 3)
    {
        if(x>-w/2 && x<w/64)
        {
            y = (m_currentLevel/100)*(h/6)*(cos(x*4*PI/w-PI)+1);
            y = 0.8*(sin(4/w*PI*x+m_offset3)+1.6)*y;
        }
    }
    return y;
}
