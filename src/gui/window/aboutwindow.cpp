#include "aboutwindow.h"
#include "global_define.h"

#include <DGuiApplicationHelper>

#include <QApplication>
#include <QFontMetrics>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
using namespace uos_ai;

const QString AboutWindow::RECORD_NUMBER_TEXT = "BeiJing-UOSAIZhiNengZhuShou-20250312S0018";

AboutWindow::AboutWindow(QWidget *parent)
    : DAboutDialog(parent)
{
    setProductName("UOS AI");
    setProductIcon(QIcon::fromTheme(getApplicationIconName()));
    setVersion(QApplication::applicationVersion());
    setDescription(tr("UOS AI is a desktop smart assistant, your personal assistant! You can communicate with it using text or voice, and it can help answer questions, provide information, and generate images based on your descriptions."));

    setupDialogContent();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, &AboutWindow::onFontChanged);
}

AboutWindow::~AboutWindow()
{
}

void AboutWindow::showDialog()
{
    onFontChanged();
    moveToCenter();
    show();
    activateWindow();
}

void AboutWindow::setupDialogContent()
{
    m_recordNumLabel = findChild<QLabel *>("LicenseLabel");
    if (m_recordNumLabel) {
        qCDebug(logAIGUI) << "licenseLabel FOUND";
        QList<QLabel *> list = findChildren<QLabel *>();
        for (int i = 0; i < list.size(); i++) {
            if (list[i] == m_recordNumLabel && i > 0) {
                qCDebug(logAIGUI) << "licenseTipLabel FOUND";
                m_recordTitleLabel = list[i - 1];
                break;
            }
        }
    }

    if (m_recordTitleLabel && m_recordNumLabel) {
        m_recordTitleLabel->setText(tr("Filing Information"));
        setLicense(RECORD_NUMBER_TEXT);
        setFixedHeight(height() + 50);
    }
}

void AboutWindow::adjustRecordNumberLabel()
{
    if (m_recordNumLabel == nullptr) {
        return;
    }

    QString tempText = RECORD_NUMBER_TEXT;
    QFontMetrics fm(m_recordNumLabel->font());
    while (fm.horizontalAdvance(tempText) > m_recordNumLabel->width()) {
        tempText.remove(tempText.length() - 1, 1);
    }
    if (tempText != RECORD_NUMBER_TEXT) {
        m_recordNumLabel->setText(tempText + "\n" + RECORD_NUMBER_TEXT.right(RECORD_NUMBER_TEXT.length() - tempText.length()));
    } else {
        m_recordNumLabel->setText(tempText);
    }
}

void AboutWindow::onFontChanged()
{
    adjustRecordNumberLabel();
}
