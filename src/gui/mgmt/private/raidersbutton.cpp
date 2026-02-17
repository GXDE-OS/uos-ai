#include "raidersbutton.h"
#include "uosfreeaccounts.h"
#include "utils/util.h"

#include <DSysInfo>

#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>

UOSAI_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

static constexpr char GIFT[] = ":/assets/images/book.svg";

RaidersButton::RaidersButton(QWidget *parent): QWidget(parent)
{
    setFixedHeight(34);
    DFontSizeManager::instance()->bind(this, DFontSizeManager::T8, QFont::Medium);
    this->hide();
    connect(QApplication::instance(), SIGNAL(fontChanged(const QFont &)), this, SLOT(onUpdateSystemFont(const QFont &)));
    resetUrl();
}

void RaidersButton::setText(const QString &text)
{
    m_text = text;
    onUpdateSystemFont(font());
}

void RaidersButton::setUrl(const QString &url)
{
    m_url = url;
}

void RaidersButton::onUpdateSystemFont(const QFont &)
{
    QFontMetrics fm(font());
    setFixedWidth(30 + 6 + fm.horizontalAdvance(m_text) + 20);
}

void RaidersButton::paintEvent(QPaintEvent *)
{
    if (m_text.isEmpty() || m_url.isEmpty()) return;

    DPalette parentPb = DGuiApplicationHelper::instance()->applicationPalette();
    QColor backgroudColor = parentPb.color(DPalette::Normal, DPalette::ObviousBackground);
    QColor highlightColor = parentPb.color(DPalette::Normal, DPalette::Highlight);
    QColor textColor = parentPb.color(DPalette::Normal, DPalette::BrightText);
    textColor.setAlphaF(0.7);

    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    int radius = 15;
    QRect rect = this->rect();
    QPainterPath path(QPointF(rect.x(), rect.y())); // 左上角
    path.lineTo(rect.topRight() - QPointF(radius, 0));  // 右上角
    path.arcTo(QRectF(rect.topRight() - QPointF(2 * radius, 0), QSizeF(2 * radius, height() - 1)), -90, 180); // 右上角的圆弧
    path.lineTo(rect.bottomRight() - QPointF(radius, 0));  // 右下角
    path.lineTo(rect.bottomLeft());  // 左下角
    path.lineTo(rect.topLeft());  // 左上角
    painter.setBrush(backgroudColor);
    painter.setPen(Qt::NoPen);
    painter.drawPath(path);

    painter.setBrush(highlightColor);
    QRect ellipesRect(10, 7, 20, 20);
    painter.drawEllipse(ellipesRect);
    QRect imageRect(ellipesRect.x() + (20 - 10) / 2, ellipesRect.y() + (20 - 10) / 2, 10, 10);
    painter.drawImage(imageRect, QImage(GIFT).scaled(10, 10));

    QRect textRect(ellipesRect.right() + 6, 0, width() - 30 - 6 - 20, height());
    painter.setPen(textColor);
    QTextOption textOption = Qt::AlignVCenter | Qt::AlignLeft;
    textOption.setWrapMode(QTextOption::NoWrap);
    painter.drawText(textRect, m_text, textOption);
}

void RaidersButton::mousePressEvent(QMouseEvent *event)
{
    QDateTime currentTime = QDateTime::currentDateTime();
    if (!m_lastDateTime.isValid() || m_lastDateTime.msecsTo(currentTime) >= 1000) { // 限制点击频率为1秒
        m_lastDateTime = currentTime;
        qCInfo(logAIGUI) << "Opening raiders URL:" << m_url;
        QDesktopServices::openUrl(QUrl(m_url));
    }

    QWidget::mousePressEvent(event);
}

void RaidersButton::resetUrl()
{
    qCDebug(logAIGUI) << QString("Resetting URL - productType(%1) language(%2) script(%3)")
                         .arg(Dtk::Core::DSysInfo::productType())
                         .arg(QLocale::system().language())
                         .arg(QLocale::system().script());
    m_watcher.reset(new QFutureWatcher<QNetworkReply::NetworkError>);
    QFuture<QNetworkReply::NetworkError> future = QtConcurrent::run([ = ] {
        return UosFreeAccounts::instance().freeAccountButtonDisplay("detail", m_hasActivity);
    });
    m_watcher->setFuture(future);
    connect(m_watcher.data(), &QFutureWatcher<QNetworkReply::NetworkError>::finished, this, [ = ]() {
        if (QNetworkReply::NoError == m_watcher.data()->future().result() && 0 != m_hasActivity.display) {
            qCInfo(logAIGUI) << "Successfully retrieved button display info, showing button";
            this->show();
            // 社区版+英文环境，获取账号的网址不同，此处写死
            if (Dtk::Core::DSysInfo::productType() == Dtk::Core::DSysInfo::Deepin && QLocale::English == QLocale::system().language()) {
                static const QString URL("https://www.deepin.org/en/uos-ai-model-configuration-guide/");
                setUrl(URL);
                setText(m_hasActivity.buttonNameEnglish);
                qCDebug(logAIGUI) << "Set English community URL:" << URL;
                return;
            }

            setUrl(m_hasActivity.url);
            setText(QLocale::Chinese == QLocale::system().language() && QLocale::SimplifiedChineseScript == QLocale::system().script() ? m_hasActivity.buttonNameChina : m_hasActivity.buttonNameEnglish);
            qCDebug(logAIGUI) << "Set URL:" << m_hasActivity.url;
        } else {
            qCWarning(logAIGUI) << "Failed to retrieve button display info or display flag is 0, hiding button";
            this->hide();
        }
    });
}
