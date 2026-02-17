#!/bin/bash

# 判断 DeepinAIAssistant 是否存在
if [ -f "/usr/bin/DeepinAIAssistant" ]; then
    # 调用 dbus-send 接口
    dbus-send --print-reply --dest=com.iflytek.aiassistant /aiassistant/deepinmain com.iflytek.aiassistant.mainWindow.TextToTranslate
else
    # 判断 uos-ai-assistant 是否存在
    if [ -f "/usr/bin/uos-ai-assistant" ]; then
        # 调用 dbus-send 接口
        dbus-send --print-reply --dest=com.deepin.copilot /com/deepin/copilot com.deepin.copilot.textTranslation
    else
        echo "Both DeepinAIAssistant and uos-ai-assistant do not exist."
    fi
fi