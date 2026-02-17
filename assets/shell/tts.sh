#!/bin/bash

SERVICE="com.iflytek.aiassistant"
TTS_OBJECT_PATH="/aiassistant/tts"
TTS_INTERFACE="com.iflytek.aiassistant.tts"

MAIN_OBJECT_PATH="/aiassistant/deepinmain"
MAIN_INTERFACE="com.iflytek.aiassistant.mainWindow"


ISWORK_METHOD="isTTSInWorking"
STOP_METHOD="stopTTSDirectly" 
START_METHOD="TextToSpeech"

reply=$(dbus-send --session --dest="$SERVICE" --print-reply --type=method_call \
    "$TTS_OBJECT_PATH" "$TTS_INTERFACE.$ISWORK_METHOD")

if [ -z "$reply" ]; then
    echo "No response from DBus."
    exit 1
fi

result=$(echo "$reply" | sed -n '2p' | xargs)

if [ "$result" = "boolean true" ]; then
    echo "TTS is working."
    reply=$(dbus-send --session --dest="$SERVICE" --print-reply --type=method_call \
    "$TTS_OBJECT_PATH" "$TTS_INTERFACE.$STOP_METHOD")
fi
reply=$(dbus-send --session --dest="$SERVICE" --print-reply --type=method_call \
"$MAIN_OBJECT_PATH" "$MAIN_INTERFACE.$START_METHOD")