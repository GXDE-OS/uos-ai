#include "xclipboard.h"

#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include <QTimer>

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logWordWizard)

UOSAI_USE_NAMESPACE

XClipboard::XClipboard(QObject *parent) : BaseClipboard(parent)
{
    selectClip = QApplication::clipboard();
    connect(selectClip, &QClipboard::selectionChanged, [this]() {
        clipText = selectClip->text(QClipboard::Mode::Selection).trimmed();
        if(!clipText.isEmpty()) {
//            qInfo() << "clipText is" << clipText;
            if (clipText == "\u0001" || clipText == "￼") { // 划到图片，不显示划词工具栏（wps：\u0001）（其他：￼）
                qCInfo(logWordWizard) << "INVALID text:" << clipText;
                selectClip->clear(QClipboard::Mode::Selection);
                return;
            }
            emit selectWords();
        }
    });
}

QString XClipboard::getClipText()
{
    return clipText;
}

void XClipboard::clearClipText()
{
    clipText = "";
}

void XClipboard::setClipText(const QString &text)
{
    clipText = text;
}

bool XClipboard::isScribeWordsVisible()
{
    return !clipText.trimmed().isEmpty();
}

void XClipboard::blockChangedSignal(bool block)
{
    selectClip->blockSignals(block);
}
