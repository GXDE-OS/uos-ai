#include "uosai.h"
#include <fstream>
#include <fcitx-utils/dbus/message.h>
#include <fcitx-utils/dbus/objectvtable.h>
#include <fcitx-utils/metastring.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx-utils/misc.h>
#include <fcitx-module/dbus/dbus_public.h>
#include <fcitx-utils/log.h>
#include <string>

#define FCITX_INPUTMETHOD_DBUS_INTERFACE "org.fcitx.Fcitx5.uosai"
#define FCITX_INPUTCONTEXT_DBUS_INTERFACE "org.fcitx.Fcitx5.InputContext"

namespace fcitx {

namespace {

std::vector<dbus::DBusStruct<std::string, int>>
buildFormattedTextVector(const Text &text) {
    std::vector<dbus::DBusStruct<std::string, int>> vector;
    for (size_t i = 0, e = text.size(); i < e; i++) {
        // In fcitx 4, underline bit means "no underline", so we need to reverse
        // it.
        const auto flag = text.formatAt(i) ^ TextFormatFlag::Underline;
        vector.emplace_back(
            std::make_tuple(text.stringAt(i), static_cast<int>(flag)));
    }
    return vector;
}

int getDisplayNumber(const std::string &var) {
    auto pos = var.find(':');
    if (pos == std::string::npos) {
        return 0;
    }
    // skip :
    pos += 1;
    // Handle address like :0.0
    auto period = var.find(pos, '.');
    if (period != std::string::npos) {
        period -= pos;
    }

    try {
        std::string num(var.substr(pos, period));
        int displayNumber = std::stoi(num);
        return displayNumber;
    } catch (...) {
    }
    return 0;
}
} // namespace

static inline std::string readFileContent(const std::string &file) {
    std::ifstream fin(file, std::ios::binary | std::ios::in);
    std::vector<char> buffer;
    constexpr auto chunkSize = 4096;
    do {
        auto curSize = buffer.size();
        buffer.resize(curSize + chunkSize);
        if (!fin.read(buffer.data() + curSize, chunkSize)) {
            buffer.resize(curSize + fin.gcount());
            break;
        }
    } while (0);
    std::string str{buffer.begin(), buffer.end()};
    return stringutils::trim(str);
}

static inline std::string getLocalMachineId(const std::string &fallback = {}) {
    auto content = readFileContent("/var/lib/dbus/machine-id");
    if (content.empty()) {
        content = readFileContent("/etc/machine-id");
    }

    return content.empty() ? fallback : content;
}

class UosAiInputMethod : public dbus::ObjectVTable<UosAiInputMethod> {
public:
    UosAiInputMethod(int display, UosAIFrontendModule *module, dbus::Bus *bus)
        : display_(display), module_(module), instance_(module->instance()),
          bus_(bus) {
        try {
            bus_->addObjectVTable("/uosai", FCITX_INPUTMETHOD_DBUS_INTERFACE,
                                  *this);
        } catch (const std::exception &e) {
            FCITX_ERROR() << "UosAi: Exception during DBus setup for display " << display << ": " << e.what();
            throw;
        } catch (...) {
            FCITX_ERROR() << "UosAi: Unknown exception during DBus setup for display " << display;
            throw;
        }

        auto localMachineId = getLocalMachineId(/*fallback=*/"machine-id");
        auto path = stringutils::joinPath(
            "fcitx", "dbus", stringutils::concat(localMachineId, "-", display));
        bool res = StandardPath::global().safeSave(
            StandardPath::Type::Config, path, [this](int fd) {
                auto address = bus_->address();
                fs::safeWrite(fd, address.c_str(), address.size() + 1);
                // Because fcitx5 don't launch dbus by itself, we write 0
                // on purpose to make address resolve fail, except WPS.
                pid_t pid = 0;
                fs::safeWrite(fd, &pid, sizeof(pid_t));
                fs::safeWrite(fd, &pid, sizeof(pid_t));
                return true;
            });
        if (res) {
            // Failed to write address file does not matter if we could use
            // regular dbus.
            pathWrote_ =
                stringutils::joinPath(StandardPath::global().userDirectory(
                                          StandardPath::Type::Config),
                                      path);
        } else {
            FCITX_WARN() << "UosAi: Failed to write DBus address file for display " << display;
        }
        FCITX_INFO() << "UosAi: UosAi input method initialization completed for display " << display;
    }

    ~UosAiInputMethod() override {
        FCITX_INFO() << "UosAi: Destroying UosAi input method for display " << display_;
        if (!pathWrote_.empty()) {
            unlink(pathWrote_.data());
        }
    }

    std::tuple<int, bool, uint32_t, uint32_t, uint32_t, uint32_t>
    createICv3(const std::string &appname, int pid);
    void commitText(const std::string &text);
    void deleteChar();
    void setPreEditOn(bool isOn);
    void commitToPreEdit(const std::string &text);
    dbus::Bus *bus() { return bus_; }
    auto *instance() { return module_->instance(); }
    auto *parent() { return module_; }

    FCITX_OBJECT_VTABLE_SIGNAL(signalFocusIn, "SignalFocusIn", "");
    FCITX_OBJECT_VTABLE_SIGNAL(signalFocusOut, "SignalFocusOut", "");

private:
    // V1 and V2 are too old, so we just ignore them.
    FCITX_OBJECT_VTABLE_METHOD(createICv3, "CreateICv3", "si", "ibuuuu");
    FCITX_OBJECT_VTABLE_METHOD(commitText, "PaddingString", "s", "");
    FCITX_OBJECT_VTABLE_METHOD(deleteChar, "DeleteChar", "", "");
    FCITX_OBJECT_VTABLE_METHOD(setPreEditOn, "SetPreEditOn", "b", "");
    FCITX_OBJECT_VTABLE_METHOD(commitToPreEdit, "CommitToPreEdit", "s", "");
 
    int display_;
    UosAIFrontendModule *module_;
    Instance *instance_;
    dbus::Bus *bus_;
    std::string pathWrote_;
};

class Fcitx4InputContext : public InputContext,
                           public dbus::ObjectVTable<Fcitx4InputContext> {
public:
    Fcitx4InputContext(int id, InputContextManager &icManager,
                       UosAiInputMethod *im, const std::string &sender,
                       const std::string &program)
        : InputContext(icManager, program),
          path_(stringutils::concat("/inputcontext_", id)), im_(im),
          handler_(im_->parent()->serviceWatcher().watchService(
              sender,
              [this](const std::string &, const std::string &,
                     const std::string &newName) {
                  if (newName.empty()) {
                      delete this;
                  }
              })),
          name_(sender) {
        created();
    }

    ~Fcitx4InputContext() override {
        InputContext::destroy(); 
    }

    const char *frontend() const override { return "fcitx4"; }

    const dbus::ObjectPath &path() const { return path_; }

    void updateIM(const InputMethodEntry *entry) {
        currentIMTo(name_, entry->name(), entry->uniqueName(),
                    entry->languageCode());
    }

    void commitStringImpl(const std::string &text) override {
        commitStringDBusTo(name_, text);
    }

    void updatePreeditImpl() override {
        auto preedit =
            im_->instance()->outputFilter(this, inputPanel().clientPreedit());
        std::vector<dbus::DBusStruct<std::string, int>> strs =
            buildFormattedTextVector(preedit);
        updateFormattedPreeditTo(name_, strs, preedit.cursor());
    }

    void deleteSurroundingTextImpl(int offset, unsigned int size) override {
        deleteSurroundingTextDBusTo(name_, offset, size);
    }

    void forwardKeyImpl(const ForwardKeyEvent &key) override {
        forwardKeyDBusTo(name_, static_cast<uint32_t>(key.rawKey().sym()),
                         static_cast<uint32_t>(key.rawKey().states()),
                         key.isRelease() ? 1 : 0);
        bus()->flush();
    }
#define CHECK_SENDER_OR_RETURN                                                 \
    if (currentMessage()->sender() != name_)                                   \
    return

    void enableInputContext() {}

    void closeInputContext() {}

    void mouseEvent(int) {}

    void setCursorLocation(int x, int y) {
        CHECK_SENDER_OR_RETURN;
        setCursorRect(Rect{x, y, 0, 0});
    }

    void focusInDBus() {
        CHECK_SENDER_OR_RETURN;
        focusIn();
    }

    void focusOutDBus() {
        CHECK_SENDER_OR_RETURN;
        focusOut();
    }

    void resetDBus() {
        CHECK_SENDER_OR_RETURN;
        reset();
    }

    void setCursorRectDBus(int x, int y, int w, int h) {
        CHECK_SENDER_OR_RETURN;
        setCursorRect(Rect{x, y, x + w, y + h});
    }

    void setCapability(uint32_t cap) {
        CHECK_SENDER_OR_RETURN;
        setCapabilityFlags(CapabilityFlags{cap});
    }

    void setSurroundingText(const std::string &str, uint32_t cursor,
                            uint32_t anchor) {
        CHECK_SENDER_OR_RETURN;
        surroundingText().setText(str, cursor, anchor);
        updateSurroundingText();
    }

    void setSurroundingTextPosition(uint32_t cursor, uint32_t anchor) {
        CHECK_SENDER_OR_RETURN;
        surroundingText().setCursor(cursor, anchor);
        updateSurroundingText();
    }

    void destroyDBus() {
        CHECK_SENDER_OR_RETURN;
        delete this;
    }

    int processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state,
                        int isRelease, uint32_t time) {
        CHECK_SENDER_OR_RETURN false;
        KeyEvent event(
            this, Key(static_cast<KeySym>(keyval), KeyStates(state), keycode),
            isRelease, time);
        // Force focus if there's keyevent.
        if (!hasFocus()) {
            focusIn();
        }

        bool handled = keyEvent(event);
        return handled ? 1 : 0;
    }

private:
    // Because there is no application to use, don't impl CommitPreedit and
    // UpdateClientSideUI.
    FCITX_OBJECT_VTABLE_METHOD(enableInputContext, "EnableIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(closeInputContext, "CloseIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusInDBus, "FocusIn", "", "");
    FCITX_OBJECT_VTABLE_METHOD(focusOutDBus, "FocusOut", "", "");
    FCITX_OBJECT_VTABLE_METHOD(resetDBus, "Reset", "", "");
    FCITX_OBJECT_VTABLE_METHOD(mouseEvent, "MouseEvent", "i", "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorLocation, "SetCursorLocation", "ii",
                               "");
    FCITX_OBJECT_VTABLE_METHOD(setCursorRectDBus, "SetCursorRect", "iiii", "");
    FCITX_OBJECT_VTABLE_METHOD(setCapability, "SetCapacity", "u", "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingText, "SetSurroundingText", "suu",
                               "");
    FCITX_OBJECT_VTABLE_METHOD(setSurroundingTextPosition,
                               "SetSurroundingTextPosition", "uu", "");
    FCITX_OBJECT_VTABLE_METHOD(destroyDBus, "DestroyIC", "", "");
    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuuiu",
                               "i");

    FCITX_OBJECT_VTABLE_SIGNAL(commitStringDBus, "CommitString", "s");
    FCITX_OBJECT_VTABLE_SIGNAL(currentIM, "CurrentIM", "sss");
    FCITX_OBJECT_VTABLE_SIGNAL(updateFormattedPreedit, "UpdateFormattedPreedit",
                               "a(si)i");
    FCITX_OBJECT_VTABLE_SIGNAL(deleteSurroundingTextDBus,
                               "DeleteSurroundingText", "iu");
    FCITX_OBJECT_VTABLE_SIGNAL(forwardKeyDBus, "ForwardKey", "uui");

    dbus::ObjectPath path_;
    UosAiInputMethod *im_;
    std::unique_ptr<HandlerTableEntry<dbus::ServiceWatcherCallback>> handler_;
    std::string name_;
};

std::tuple<int, bool, uint32_t, uint32_t, uint32_t, uint32_t>
UosAiInputMethod::createICv3(const std::string &appname, int /*pid*/) {
    auto sender = currentMessage()->sender();
    int icid = module_->nextIcIdx();
    auto *ic = new Fcitx4InputContext(icid, instance_->inputContextManager(),
                                      this, sender, appname);
    auto group =
        instance_->defaultFocusGroup(stringutils::concat("x11::", display_));
    if (!group) {
        group = instance_->defaultFocusGroup("x11:");
    }
    ic->setFocusGroup(group);
    bus_->addObjectVTable(ic->path().path(), FCITX_INPUTCONTEXT_DBUS_INTERFACE,
                          *ic);

    return std::make_tuple(icid, true, 0, 0, 0, 0);
}

void UosAiInputMethod::commitText(const std::string &text) {
    InputContext *ic = instance_->inputContextManager().lastFocusedInputContext();
    if(ic && ic->hasFocus()) {
        ic->commitString(text);
    } else {
        FCITX_ERROR() << "UosAi: No focused input context or context lost focus";
    }
}

void UosAiInputMethod::deleteChar()
{
    InputContext *ic = instance_->inputContextManager().lastFocusedInputContext();
    if (ic && ic->hasFocus()) {
        ic->deleteSurroundingText(-1, 1);
    } else {
        FCITX_ERROR() << "UosAi: No focused input context or context lost focus";
    }
}

void UosAiInputMethod::setPreEditOn(bool isOn)
{
    InputContext *ic = instance_->inputContextManager().lastFocusedInputContext();
    if (ic && ic->hasFocus()) {
        if (ic->isPreeditEnabled() == isOn) {
            return;
        }

        ic->setEnablePreedit(isOn);
        if (!isOn) {
            InputPanel &inputPanel = ic->inputPanel();
            std::string preedit = inputPanel.clientPreedit().toString();
            inputPanel.reset();
            ic->commitString(preedit);
        }
    } else {
        FCITX_ERROR() << "UosAi: No focused input context or context lost focus";
    }
}

void UosAiInputMethod::commitToPreEdit(const std::string &text)
{
    InputContext *ic = instance_->inputContextManager().lastFocusedInputContext();
    if (ic && ic->hasFocus()) {
        if (!ic->isPreeditEnabled()) {
            ic->setEnablePreedit(true);
        }

        InputPanel &inputPanel = ic->inputPanel();
        inputPanel.setClientPreedit(Text(text));
        ic->updatePreedit();
    } else {
        FCITX_ERROR() << "UosAi: No focused input context or context lost focus";
    }
}

UosAIFrontendModule::UosAIFrontendModule(Instance *instance)
    : instance_(instance),
      table_(
          [this](int display) {
              try {
                  if (!dbus()) {
                      FCITX_ERROR() << "UosAi: DBus module not available for display " << display;
                      return false;
                  }
                  
                  auto *dbusInstance = dbus();
                  auto *busPtr = bus();
                  
                  if (!busPtr) {
                      FCITX_ERROR() << "UosAi: DBus bus is NULL for display " << display;
                      return false;
                  }

                  uosaiInputMethod_.emplace(
                      display, std::make_unique<UosAiInputMethod>(
                                   display, this, busPtr));
                  return true;
              } catch (const std::exception &e) {
                  FCITX_ERROR() << "UosAi: Failed to create input method for display " << display << ": " << e.what();
              } catch (...) {
                  FCITX_ERROR() << "UosAi: Failed to create input method for display " << display;
              }
              return false;
          },
          [this](int display) {
              uosaiInputMethod_.erase(display); 
          }) {

    FCITX_INFO() << "UosAi: Initializing UosAI frontend module";

    if (!dbus()) {
        FCITX_ERROR() << "UosAi: DBus module not available, cannot initialize";
        return;
    }
    
    auto *busPtr = bus();
    if (!busPtr) {
        FCITX_ERROR() << "UosAi: DBus bus is NULL, cannot initialize";
        return;
    }

    watcher_ = std::make_unique<dbus::ServiceWatcher>(*busPtr);

    // Always create display number zero.
    addDisplay("");

    event_ = instance_->watchEvent(
        EventType::InputContextInputMethodActivated, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &activated = static_cast<InputMethodActivatedEvent &>(event);
            auto *ic = activated.inputContext();
            if (ic->frontendName() == "fcitx4") {
                if (const auto *entry = instance_->inputMethodManager().entry(
                        activated.name())) {
                    static_cast<Fcitx4InputContext *>(ic)->updateIM(entry);
                }
            }
        });

    //焦点进入事件
    focusInEvent_ = instance_->watchEvent(
        fcitx::EventType::InputContextFocusIn,
        fcitx::EventWatcherPhase::Default,
        [this](fcitx::Event &event) {
            auto &focusInEvent = static_cast<fcitx::FocusInEvent &>(event);
            this->onFocusIn(focusInEvent);
        });

    // 焦点离开事件
    focusOutEvent_ = instance_->watchEvent(
        fcitx::EventType::InputContextFocusOut,
        fcitx::EventWatcherPhase::Default,
        [this](fcitx::Event &event) {
            auto &focusOutEvent = static_cast<fcitx::FocusOutEvent &>(event);
            this->onFocusOut(focusOutEvent);
        });

    FCITX_INFO() << "UosAi: UosAI frontend module initialization completed";
}

dbus::Bus *UosAIFrontendModule::bus() {
    return dbus()->call<IDBusModule::bus>();
}

void UosAIFrontendModule::addDisplay(const std::string &name) {
    displayToHandle_.emplace(name, table_.add(getDisplayNumber(name), name));
}

void UosAIFrontendModule::removeDisplay(const std::string &name) {
    displayToHandle_.erase(name);
}

void UosAIFrontendModule::onFocusIn(const fcitx::FocusInEvent &event) {
    // 获取当前激活的 InputContext
    InputContext *ic = event.inputContext();
    if (ic && ic->hasFocus()) {
        for (auto &pair : uosaiInputMethod_) {        
            pair.second->signalFocusIn();
        }
    }
}

void UosAIFrontendModule::onFocusOut(const fcitx::FocusOutEvent &event) {
    // 获取当前激活的 InputContext
    InputContext *ic = event.inputContext();
    if (ic && !ic->hasFocus()) {
        for (auto &pair : uosaiInputMethod_) {
            pair.second->signalFocusOut();
        }
    }
}

class UosAIFrontendModuleFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new UosAIFrontendModule(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::UosAIFrontendModuleFactory);
