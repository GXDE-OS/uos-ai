export enum AudioEvent {
    AeError = -1, // 错误
    AeSuccess = 0, // 成功
    AeLevelUpdated = 1, // 录音音量级别更新
    AeTextReceived = 2, // 语音识别文本接收
    AePlayFinished = 3, // 音频播放完成
    AeRecordError = 4, // 录音错误
    AePlayerError = 5, // 播放错误
    AePlayDeviceChanged = 6, // 播放设备变化
    AeRecordDeviceChanged = 7, // 录音设备变化
}

// 数字人状态类型
export enum DigitalHumanState {
    Silence = "silence",
    Listen = "listen",
    Thinking = "thinking",
    Answer = "answer",
    Error = "error",
}
