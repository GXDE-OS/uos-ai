#ifndef _FCITX5_FRONTEND_UOSAI_UOSAI_H_
#define _FCITX5_FRONTEND_UOSAI_UOSAI_H_

#include <fcitx-utils/dbus/servicewatcher.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/instance.h>
// #include <config.h>

namespace fcitx {

class UosAiInputMethod;

class UosAIFrontendModule : public AddonInstance {
public:
    UosAIFrontendModule(Instance *instance);

    dbus::Bus *bus();
    Instance *instance() { return instance_; }
    int nextIcIdx() { return ++icIdx_; }

    dbus::ServiceWatcher &serviceWatcher() { return *watcher_; }

    void addDisplay(const std::string &name);
    void removeDisplay(const std::string &name);
    void onFocusIn(const fcitx::FocusInEvent &event);
    void onFocusOut(const fcitx::FocusOutEvent &event);

private:
    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    Instance *instance_;
    // A display number to dbus object mapping.
    std::unordered_map<int, std::unique_ptr<UosAiInputMethod>>
        uosaiInputMethod_;

    // Use multi handler table as reference counter like display number ->
    // display name mapping.
    MultiHandlerTable<int, std::string> table_;
    std::unordered_map<std::string,
                       std::unique_ptr<HandlerTableEntry<std::string>>>
        displayToHandle_;

    std::unique_ptr<HandlerTableEntry<EventHandler>> event_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> focusInEvent_;
    std::unique_ptr<HandlerTableEntry<EventHandler>> focusOutEvent_;
    int icIdx_ = 0;
    std::unique_ptr<dbus::ServiceWatcher> watcher_;
};
} // namespace fcitx

#endif // _FCITX5_FRONTEND_UOSAI_UOSAI_H_
