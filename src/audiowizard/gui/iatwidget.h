#ifndef IATWIDGET_H
#define IATWIDGET_H

#include <uosai_global.h>
#include "audiowizard/private/voiceanimationwidget.h"
#include "audio/audiorecorder.h"
#include "audiowizard/modelwrapper/common/iatmodel.h"

#include <DWidget>
#include <DBlurEffectWidget>
#include <DWindowCloseButton>
#include <DLabel>

#include <QResizeEvent>
#include <QPaintEvent>
#include <QTimer>

namespace uos_ai {
class IatWidget : public DTK_WIDGET_NAMESPACE::DWidget {
    Q_OBJECT
public:
    static int getRecoderVolume();

    explicit IatWidget(QObject *parent = nullptr);
    ~IatWidget();
    bool startIat();
    bool stopIat();
    bool isWorking() { return true; }

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void initUi();
    void initConnect();
    void onUpdateSystemTheme();
    void onBtClicked();
    void onAudioData(QByteArray data);
    void onOpenConfigDialog();
    void onMouseClicked(int type, int x, int y);
    void onKeyPressed(int type);
    void onFocusIn();
    void onFocusOut();
    void onTextReceived(QString text, bool isEnd);
    //void onNetworkStateChanged(bool isOnline);

private:
    DTK_WIDGET_NAMESPACE::DBlurEffectWidget  *m_effectWidget = nullptr;
    DTK_WIDGET_NAMESPACE::DWindowCloseButton *m_closeBt      = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_infoLabel                = nullptr;
    DTK_WIDGET_NAMESPACE::DLabel *m_warnLabel                = nullptr;

    QObject *m_parent                       = nullptr;
    VoiceAnimationWidget *m_animationWidget = nullptr;
    AudioRecorder *m_recorder               = nullptr;
    IatModel *m_model                       = nullptr;
    QTimer *m_timer                         = nullptr;

    int m_volume = 0;
    QString m_text;
    long long m_initMs = 0;
    volatile bool m_isCanceled = false;
    bool m_isOnline = true;
};
}

#endif //IATWIDGET_H
