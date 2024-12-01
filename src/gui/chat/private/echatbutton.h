// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef ECHATBUTTON_H
#define ECHATBUTTON_H

#include <DIconButton>

DWIDGET_USE_NAMESPACE

class LIBDTKWIDGETSHARED_EXPORT EChatButton : public DIconButton
{
    Q_OBJECT
public:
    EChatButton(QWidget * parent = nullptr);

    void updateActiveStatus(bool isActive);

    QSize sizeHint() const override;

protected:
    void initStyleOption(DStyleOptionButton *option) const override;
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

    QPixmap getIcon();

private:
    QIcon m_icon = QIcon(QString(":/icons/deepin/builtin/dark/icons/chat.svg"));
    bool m_isHover = false;
    bool m_isPress = false;
    bool m_isActive = false;
};

#endif // ECHATBUTTON_H
