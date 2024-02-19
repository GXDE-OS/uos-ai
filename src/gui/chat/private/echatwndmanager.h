#ifndef ECHATWNDMANAGER_H
#define ECHATWNDMANAGER_H
#include "esingleton.h"

#include <QObject>
#include <QList>

#include <unordered_map>

class EChatWndManager : public QObject
{
    Q_OBJECT

    //Singletonize EChatWndManager
    SINGLETONIZE_CLASS(EChatWndManager)

    explicit EChatWndManager(QObject *parent = nullptr);
public:

public slots:
    void registeWindow(QWidget *w);
    void unregisteWindow(QWidget *w);
signals:
    void modalStateChanged(bool modalState);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void updateModalWindow(QWidget *w, bool visable);
protected:
    struct WinInfo {
        WinInfo(QWidget *w);
        virtual ~WinInfo();
        WinInfo(const WinInfo &);

        bool operator == (const WinInfo &other) const;

        QWidget *window { nullptr};
        bool isModal {false};
    };

    //Overload == make (QWidget* == WinInfo& other)
    friend bool operator == (QWidget *window, const WinInfo &other);

    std::unordered_map<QWidget *, WinInfo> m_windowMap;

    struct EModalStack {
        using ModalStack = QList<QWidget *>;

        ModalStack s;

        inline int  find(QWidget *w);
        void top(QWidget *w);
        void remove(QWidget *w);
        int count();
    } m_modalStack;;
};

#define EWndManager() (ESingleton<EChatWndManager>::getInstance())
#endif // ECHATWNDMANAGER_H
