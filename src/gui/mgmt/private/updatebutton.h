#ifndef UPDATEBUTTON_H
#define UPDATEBUTTON_H
#include "uosai_global.h"

#include <DSuggestButton>

namespace uos_ai {

class UpdateButton : public DTK_WIDGET_NAMESPACE::DSuggestButton
{
    Q_OBJECT
public:
    UpdateButton(QWidget * parent = nullptr);
    void setStatus(bool isUpdate);

signals:
    void sigButtonclicked(bool);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_suggest = true;
    bool m_redpoint = false;

};
}

#endif // UPDATEBUTTON_H
