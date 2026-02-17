#ifndef TOOLTIPWIDGET_H
#define TOOLTIPWIDGET_H

#include <DGuiApplicationHelper>

#include <QWidget>
#include <QPaintEvent>
#include <QHBoxLayout>

namespace uos_ai {
class TooltipWidget : public QWidget {
    Q_OBJECT
public:
    explicit TooltipWidget(QWidget *parent = nullptr);
    ~TooltipWidget() override {};

protected:
    void paintEvent(QPaintEvent *event) override;
};
}

#endif // TOOLTIPWIDGET_H
