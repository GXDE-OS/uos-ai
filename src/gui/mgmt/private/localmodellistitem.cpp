#include "localmodellistitem.h"
#include "iconbuttonex.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

LocalModelListItem::LocalModelListItem(DWidget *parent)
    : DWidget(parent)
    , m_pProcess(new QProcess)
{
    initUI();
    initConnect();
}

LocalModelListItem::~LocalModelListItem()
{
    if (m_pProcess) {
        m_pProcess->terminate();
        m_pProcess->deleteLater();
    }
}

void LocalModelListItem::initUI()
{
    m_pLabelTheme = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelTheme, DFontSizeManager::T6, QFont::Medium);
    m_pLabelSummary = new DLabel;
    DFontSizeManager::instance()->bind(m_pLabelSummary, DFontSizeManager::T8, QFont::Normal);
    m_pBtnUninstall = new IconButtonEx(tr("uninstall"));
    m_pBtnUninstall->setHighlight(true);

    QVBoxLayout *textLayout = new QVBoxLayout;
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addWidget(m_pLabelTheme, 0, Qt::AlignLeft);
    textLayout->addWidget(m_pLabelSummary, 0, Qt::AlignLeft);
    textLayout->addWidget(m_pBtnUninstall, 0, Qt::AlignLeft);

    m_pBtnSwitch = new DSwitchButton;

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->addLayout(textLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(m_pBtnSwitch, 0, Qt::AlignVCenter);

    setLayout(mainLayout);
}

void LocalModelListItem::initConnect()
{
    connect(m_pBtnUninstall, &IconButtonEx::clicked, this, &LocalModelListItem::onUninstall);
    connect(m_pBtnSwitch, &DSwitchButton::checkedChanged, this, &LocalModelListItem::signalSwitchChanged);
    connect(m_pProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this](int exitCode, QProcess::ExitStatus exitStatus) {
        qInfo() << "onUninstall " << m_appName << " code=" << exitCode << " status=" << exitStatus;
        if (exitCode == 0 && exitStatus == QProcess::ExitStatus::NormalExit) {
            emit this->signalUninstall();
        }
    });
}

void LocalModelListItem::setText(const QString &theme, const QString &summary)
{
    m_pLabelTheme->setText(theme);
    m_pLabelSummary->setText(summary);
}

void LocalModelListItem::setSwitchChecked(bool b)
{
    m_pBtnSwitch->setChecked(b);
}

void LocalModelListItem::setAppName(const QString &appName)
{
    m_appName = appName;
}

void LocalModelListItem::onUninstall()
{
    if (m_appName.isEmpty()) return;
    if (!m_pProcess->atEnd()) return;

    m_pProcess->start("pgrep", QStringList() << m_appName);
    m_pProcess->waitForFinished();

    if (m_pProcess->exitCode() == 0) {
        m_pProcess->start("pkexec", QStringList() << "sh" << "-c" << QString("killall %1 && apt remove %1").arg(m_appName));
    } else {
        m_pProcess->start("pkexec", QStringList() << "sh" << "-c" << QString("apt remove %1").arg(m_appName));
    }
    m_pProcess->write("Y\n");
}
