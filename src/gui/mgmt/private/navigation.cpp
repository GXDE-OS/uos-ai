#include "navigation.h"
#include "navigationdelegate.h"
#include "utils/util.h"

#include <DListView>
#include <DPaletteHelper>

#include <QDebug>
#include <QVBoxLayout>
#include <QListView>
#include <QStandardItemModel>
#include <QCoreApplication>
#include <QLoggingCategory>

UOSAI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)

class NavigationPrivate
{
public:
    NavigationPrivate(Navigation *parent) : q_ptr(parent) {}
    QModelIndex indexOfGroup(const QString &key) const
    {
        for (int i = 0; i < navbarModel->rowCount(); ++i) {
            auto index = navbarModel->index(i, 0);
            if (index.data(NavigationDelegate::NavKeyRole).toString() == key) {
                return index;
            }
        }

        return QModelIndex();
    }

    // 返回所有符合条件的key集合, e.g: key: abc, 可返回{abc.dc, abc}, 过滤掉{abcd}
    QList<QModelIndex> indexesOfGroup(const QString &key) const
    {
        static const QChar SplitChar = '.'; // group之间的key以'.'分割连接组成实际的key值
        QList<QModelIndex> res;
        for (int i = 0; i < navbarModel->rowCount(); ++i) {
            auto index = navbarModel->index(i, 0);
            const auto& navKey = index.data(NavigationDelegate::NavKeyRole).toString();
            if (navKey.startsWith(key)) {
                const auto& remainderKey = navKey.mid(key.size());
                if (remainderKey.isEmpty() || remainderKey.at(0) == SplitChar) {
                    res.append(index);
                }
            }
        }

        return res;
    }

    DListView           *navbar         = nullptr;
    QStandardItemModel  *navbarModel    = nullptr;

    Navigation *q_ptr;
    Q_DECLARE_PUBLIC(Navigation)
};

Navigation::Navigation(QWidget *parent) :
    QFrame(parent), d_ptr(new NavigationPrivate(this))
{
    Q_D(Navigation);

    setObjectName("Navigation");

    setContentsMargins(0, 0, 0, 0);
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    d->navbar = new DListView(this);
    d->navbar->setObjectName("NavigationBar");
    d->navbar->setAccessibleName("NavigationBar");
    d->navbar->setContentsMargins(0, 0, 0, 0);
    d->navbar->setAutoFillBackground(true);
    d->navbar->setViewportMargins(10, 0, 10, 0);
    DPalette pa = DPaletteHelper::instance()->palette(d->navbar);
    pa.setBrush(DPalette::ItemBackground, Qt::transparent);
    DPaletteHelper::instance()->setPalette(d->navbar, pa);

    d->navbar->setSelectionMode(QListView::SingleSelection);
    d->navbar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    d->navbarModel = new QStandardItemModel(this);

    d->navbar->setModel(d->navbarModel);

    d->navbar->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->navbar->setItemDelegate(new NavigationDelegate(d->navbar));

    layout->addWidget(d->navbar);

    connect(d->navbar->selectionModel(), &QItemSelectionModel::currentChanged, this, &Navigation::onCurrentChanged);

    if (uos_ai::Util::checkLanguage()) {
        d->navbar->setFixedWidth(180);
    } else {
        d->navbar->setFixedWidth(240);
    }
}

Navigation::~Navigation()
{
}

void Navigation::onSelectGroup(const QString &key)
{
    Q_D(Navigation);

    const QModelIndex &index = d->indexOfGroup(key);
    if (index.isValid()) {
        d->navbar->setCurrentIndex(index);
    }
}

void Navigation::updateNavigationTitles(QList<DWidget *> sortTitles)
{
    Q_D(Navigation);
    qCInfo(logAIGUI) << "Update navigation titles, count:" << sortTitles.size();
    for(auto label : sortTitles) {
        auto item = new QStandardItem;
        QString trName = label->property("title").toString();
        if (!trName.isEmpty()) {
            item->setToolTip(trName);
            item->setData(trName, Qt::DisplayRole);
            int level = label->property("level").toInt();
            item->setData(NavigationDelegate::Split + level, NavigationDelegate::NavLevelRole);
            item->setData(trName, NavigationDelegate::NavKeyRole);
            d->navbarModel->appendRow(item);
        }
    }
    d->navbar->setCurrentIndex(d->navbarModel->index(0, 0));
}

void Navigation::onCurrentChanged(const QModelIndex &current)
{
    const QString &key = current.data(NavigationDelegate::NavKeyRole).toString();
    if (!key.isEmpty()) {
        emit selectedGroup(key);
    }
}

