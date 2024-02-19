<template>
  <div
    class="main-content"
    :style="{
      '--scale': animateScale,
      '--scale1': animateScale1,
    }"
  >
    <ChatTop
      :historyLength="history.length"
      v-model:playAudioID="playAudioID"
      :recording="recording"
      :showStop="showStop"
      :videoStatus="videoStatus"
      :localmodel="currentAccount.model === 1000"
      @sigAudioASRError="sigAudioASRError"
      @routeJump="handelModel"
    />
    <div class="" style="position: relative; flex: 1 1 0">
      <div class="logo">
        <img src="../../../svg/logo.svg" class="" />
        <div>UOS AI</div>
      </div>
      <div
        class="animate-box"
        @click="activeFun(true)"
        :class="{ disabledDig: playStatus }"
      >
        <video
          autoplay
          loop
          muted
          class="video1"
          v-show="videoStatus == 1"
          @play="onPlay == 1 && !ChatWindowState"
          @pause="onPlay !== 1 || ChatWindowState"
          v-if="!ChatWindowState"
        >
          <source src="../../../assets/video/silence.webm" type="video/webm" />
        </video>
        <div class="listen" v-show="videoStatus == 2 || videoStatus == 7">
          <div class="scale">
            <img
              src="../../../assets/images/listen2.png"
              class="listen-img rotate"
            />
          </div>
          <div class="scale1">
            <img
              src="../../../assets/images/listen1.png"
              class="listen-img rotate"
            />
          </div>
        </div>
        <video
          autoplay
          loop
          muted
          class="video2"
          v-show="videoStatus == 2 || videoStatus == 7"
          @play="!ChatWindowState && (onPlay == 2 || onPlay == 7)"
          @pause="(onPlay !== 2 && onPlay !== 7) || ChatWindowState"
          v-if="!ChatWindowState"
        >
          <source src="../../../assets/video/listen.webm" type="video/webm" />
        </video>
        <video
          autoplay
          loop
          muted
          class="video3"
          v-show="videoStatus == 3"
          @play="onPlay == 3 && !ChatWindowState"
          @pause="onPlay !== 3 || ChatWindowState"
          v-if="!ChatWindowState"
        >
          <source src="../../../assets/video/thinking.webm" type="video/webm" />
        </video>
        <video
          autoplay
          loop
          muted
          class="video4"
          v-show="videoStatus == 4"
          @play="onPlay == 4 && !ChatWindowState"
          @pause="onPlay !== 4 || ChatWindowState"
          v-if="!ChatWindowState"
        >
          <source src="../../../assets/video/play.webm" type="video/webm" />
        </video>
        <video
          autoplay
          muted
          class="video5"
          v-show="
            videoStatus == 5 ||
            videoStatus == 6 ||
            videoStatus == 8 ||
            videoStatus == 9 ||
            videoStatus == 10
          "
          @play="
            !ChatWindowState &&
              (onPlay == 5 ||
                onPlay == 6 ||
                onPlay == 8 ||
                onPlay == 9 ||
                onPlay == 10)
          "
          @pause="
            (onPlay !== 5 &&
              onPlay !== 6 &&
              onPlay !== 8 &&
              onPlay !== 9 &&
              onPlay !== 10) ||
              ChatWindowState
          "
          v-if="!ChatWindowState"
        >
          <source src="../../../assets/video/error.webm" type="video/webm" />
        </video>
      </div>
    </div>
    <div class="active-box">
      <div class="active-con">
        <pre ref="target" v-if="videoStatus == 8">{{
          statusLabel
        }}<span class="go-config" href="#" @click="Qrequest(chatQWeb.launchLLMConfigWindow, true)"> {{store.loadTranslations['Go to configuration']}} ></span></pre>
        <div v-else>
          <SvgIcon v-show="videoStatus == 2" icon="voice" />{{
            Array.isArray(statusLabel) ? statusLabel[0] : statusLabel
          }}
          <span
            class="dian"
            v-show="
              videoStatus == 1 ||
              videoStatus == 2 ||
              videoStatus == 3 ||
              videoStatus == 4
            "
            >...</span
          >
          <div class="fz-14" v-if="Array.isArray(statusLabel)">
            {{ statusLabel[1] }}
          </div>
        </div>
      </div>
      <div
        class="handel"
        :class="{ disabledDig: playStatus }"
        @mouseenter="handleMouseOver()"
        @mouseleave="handleMouseOut()"
        @click="activeFun()"
        v-if="
          videoStatus == 2 ||
          videoStatus == 3 ||
          videoStatus == 4 ||
          videoStatus == 7
        "
      >
        <div class="svgBg">
          <SvgIcon icon="dig-stop" />
        </div>
        <div class="svgBg-hover"></div>
        <Tips :tip="tipCon" :class="className" v-show="isTipsVisible"> </Tips>
      </div>
      <div
        class="handel"
        @click="activeFun()"
        @mouseenter="handleMouseOver()"
        @mouseleave="handleMouseOut()"
        :class="{ disabledDig: playStatus }"
        v-else
      >
        <div class="svgBg">
          <SvgIcon icon="dig-play" />
        </div>
        <div class="svgBg-hover"></div>
        <Tips :tip="tipCon" :class="className" v-show="isTipsVisible"> </Tips>
      </div>
    </div>
  </div>
</template>
<script setup>
import { useGlobalStore } from "@/store/global";
const { chatQWeb, updateActivityColor, updateTheme } = useGlobalStore();
const store = useGlobalStore();
import { Qrequest } from "@/utils";
import _ from "lodash";
import { useRouter } from "vue-router";
import ChatTop from "./ChatTop.vue";
import Tips from "../../../components/tips/tips.vue";

const instance = getCurrentInstance();
const router = useRouter();
const animateScale = ref(1); //动画缩放比例最底层
const animateScale1 = ref(1); //动画缩放比例第二层
const videoStatus = ref(2); //动画状态1:silence,2:listen,3:think,4:input,5:error
const playStatus = ref(false); //动画，播放按钮点击状态
const question = ref("");
const isAudioInput = ref(false);
const isAudioOutput = ref(false);
const playAudioID = ref("");
const AudioASRStreamState = ref(false); //区分是不是要路由跳转
const ChatWindowState = ref(false); //窗口关闭true,打开false
const onPlay = ref(2);
const tipCon = computed(() => {
  if (
    videoStatus.value == 2 ||
    videoStatus.value == 3 ||
    videoStatus.value == 4 ||
    videoStatus.value == 7
  ) {
    return useGlobalStore().loadTranslations.Stop;
  } else {
    return useGlobalStore().loadTranslations.Activate;
  }
});
const className = ref("top-right"); //tips组件tips位置类
const statusLabel = computed(() => {
  // 根据返回值枚举映射到对应的文字状态
  switch (videoStatus.value) {
    case 1:
      return [
        useGlobalStore().loadTranslations.Sleeping,
        useGlobalStore().loadTranslations[
          "Click on the animation or Ctrl+Super+C to activate"
        ],
      ];
    case 2:
      return [
        useGlobalStore().loadTranslations.Listening,
        useGlobalStore().loadTranslations[
          "Click the animation or press Enter to send"
        ],
      ];
    case 3:
      return [
        useGlobalStore().loadTranslations.Thinking,
        useGlobalStore().loadTranslations["Click animation to interrupt"],
      ];
    case 4:
      return [
        useGlobalStore().loadTranslations.Answering,
        useGlobalStore().loadTranslations["Click animation to interrupt"],
      ];
    case 5:
      return useGlobalStore().loadTranslations[
        "Connection failed, please check the network."
      ]; //播放按钮不可点击直到接收到信号满足激活条件
    case 6:
      return useGlobalStore().loadTranslations["Microphone not detected"]; //播放按钮不可点击直到接收到信号满足激活条件
    case 7:
      return useGlobalStore().loadTranslations[
        "Stop recording after %1 seconds"
      ].replace("%1", countDown.value);
    case 8:
      return errorCon.value; //播放按钮不可点击直到接收到信号满足激活条件比如无模型，账号过期或额度用完
    case 9:
      return errorCon.value; //录音中的异常
    case 10:
      return useGlobalStore().loadTranslations[
        "Sound output device not detected"
      ]; //没有输出设备
    default:
      return [
        useGlobalStore().loadTranslations.Listening,
        useGlobalStore().loadTranslations[
          "Click the animation or press Enter to send"
        ],
      ];
  }
});
const history = ref([]);
const accountList = ref([]);
const currentAccount = ref("");
const errorCon = ref("");
const showStop = ref(false); //思考回答为true
const isTipsVisible = ref(false); //tips是否显示
let tipsTimerId = null;
/**鼠标移入tips显示 */
const handleMouseOver = () => {
  tipsTimerId && clearTimeout(tipsTimerId);
  tipsTimerId = setTimeout(() => {
    isTipsVisible.value = true;
  }, 2000); // 延迟2秒
};
/**鼠标移出tips隐藏 */
const handleMouseOut = () => {
  tipsTimerId && clearTimeout(tipsTimerId);
  isTipsVisible.value = false;
};
/**初始化 */
const initChat = async () => {
  const _history = await Qrequest(chatQWeb.getAiChatRecords, false);
  const resAccount = await Qrequest(chatQWeb.queryLLMAccountList);
  const resID = await Qrequest(chatQWeb.currentLLMAccountId);
  isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable);
  isAudioOutput.value = await Qrequest(chatQWeb.isAudioOutputAvailable);
  history.value = JSON.parse(_history);
  const isNetworkAvailable = await Qrequest(chatQWeb.isNetworkAvailable);
  if (resAccount) {
    const list = JSON.parse(resAccount);
    list.forEach((element) => {
      if (element.id === resID) {
        element.active = true;
        currentAccount.value = element;
      }
    });
    accountList.value = list;
  }
  if (!isNetworkAvailable) {
    videoStatus.value = 5;
    return;
  }
  if (!isAudioInput.value) {
    videoStatus.value = 6;
  } else if (!isAudioOutput.value) {
    videoStatus.value = 10;
  } else {
    chatQWeb.playSystemSound && Qrequest(chatQWeb.playSystemSound, 0);
    handleRecorder();
    startCounter();
  }
};
//10s没有声音自动进入休眠
let count = 0; // 计数器的初始值
let timerId = null; // 用于存储计时器的ID
// 创建一个计时器，每秒更新计数器的值
const startCounter = () => {
  console.log("startCounter-------");
  timerId = setInterval(() => {
    count++;
    console.log("startCounter", count, question.value.trim().length);
    if (count == 2 && question.value.trim().length !== 0) {
      console.log("5count", "startCounter", count);
      sigAudioASRError();
    } else if (count == 10) {
      console.log("10count", count);
      sigAudioASRError();
      videoStatus.value = 1;
    }
  }, 1000); // 每秒更新一次计数器的值
};
// 销毁计时器，停止更新计数器的值
const stopCounter = () => {
  console.log("stopCounter------");
  if (timerId) {
    clearInterval(timerId);
    timerId = null;
    count = 0;
  }
};
/**录音倒计时 */
const countDown = ref(0);
const sigAudioCountDown = (res) => {
  countDown.value = res;
  if (countDown.value > 0) {
    videoStatus.value = 7;
  } else {
    sigAudioASRError();
  }
};
const recording = ref(false); //录音中为true
/**语音聆听--录音 */
const handleRecorder = async () => {
  console.log(
    999,
    "handleRecorder",
    recording.value,
    showStop.value,
    isAudioInput.value
  );
  if (!isAudioInput.value || showStop.value) return;
  if (recording.value) return sigAudioASRError();
  if (!recording.value) {
    isAudioInput.value = await Qrequest(chatQWeb.isAudioInputAvailable);
    await Qrequest(chatQWeb.stopPlayTextAudio);
    if (isAudioInput.value) {
      console.log("startRecorder-------");
      await Qrequest(chatQWeb.startRecorder);
      recording.value = true;
      playAudioID.value = "";
    }
  }
};
/**录音信号返回 */
const sigAudioASRStream = (res, isEnd) => {
  console.log("sigAudioASRStream", res, isEnd);
  question.value = res;
  if (res !== "") {
    stopCounter();
    if (videoStatus.value == 2) {
      startCounter();
    }
  }
  //因为信号有延迟所以只能加判断等信号结束再跳转把之前录音丢掉
  if (isEnd && AudioASRStreamState.value) {
    stopCounter();
    router.push("/chat");
  } else if (isEnd && videoStatus.value !== 1 && videoStatus.value !== 10) {
    sendQuestion();
  }
};
/**录音声波起伏 */
const sigAudioSampleLevel = (res) => {
  const data = 1 + (res / 100) * 0.7;
  const data1 = 1 + (res / 100) * 1.2;

  animateScale.value = data;
  animateScale1.value = data1;
};
const talkID = ref("");
const sendQuestion = async () => {
  console.log("sendQuestion", question.value);
  if (question.value.trim().length === 0 || ChatWindowState.value) {
    question.value = "";
    return;
  }
  await Qrequest(chatQWeb.stopPlayTextAudio);
  playAudioID.value = "";
  if (recording.value) sigAudioASRError();
  stopCounter();
  history.value.push({
    role: "user",
    content: question.value,
  });
  const { id, model, icon, llmname } = currentAccount.value;
  const res = await Qrequest(
    chatQWeb.sendAiRequest,
    id ? id : "",
    model ? model : "",
    1,
    question.value,
    2
  );
  history.value.push({
    role: "assistant",
    anwsers: [
      {
        content: "",
        talkID: res,
        status: 0,
        type: 0,
        llmIcon: icon,
        llmName: llmname,
      },
    ],
  });
  talkID.value = res;
  showStop.value = true;
  question.value = "";
  if (videoStatus.value !== 6 && videoStatus.value !== 10) {
    videoStatus.value = 3;
  }
};
// 接受AI文本信息
const sigAiReplyStream = (type, value, status) => {
  console.log({ type, value, status });
  if (status === 0) {
    _.last(_.last(history.value).anwsers).content =
      _.last(_.last(history.value).anwsers).content + value;
  } else if (status === 200) {
    _.last(_.last(history.value).anwsers).status = status;
    playTextAudio();
    if (videoStatus.value !== 6 && videoStatus.value !== 10) {
      videoStatus.value = 4;
    }
  } else {
    _.last(_.last(history.value).anwsers).content = value;
    _.last(history.value).status = status;
    _.last(_.last(history.value).anwsers).status = status;
  }
  if (status !== 0) {
    const _question = history.value[history.value.length - 2].content;
    const { type, content } = _.last(_.last(history.value).anwsers);
    if (status < 0) {
      if (status === -9002 || status === -9000 || status === -9001) {
        errorCon.value = content;
        videoStatus.value = 8;
      } else {
        videoStatus.value = 4;
      }
      playTextAudio();
    }

    const { icon, displayname } = currentAccount.value;
    Qrequest(
      chatQWeb.logAiChatRecord,
      talkID.value,
      _question,
      content,
      false,
      status,
      "",
      type,
      icon ? icon : "",
      displayname ? displayname : ""
    );
  }
};
/**激活静置状态切换 */
const activeFun = (res) => {
  if (playStatus.value) return;
  console.log("activeFun", videoStatus.value, res);

  if (videoStatus.value == 2 || videoStatus.value == 7) {
    if (res && question.value.trim().length !== 0) {
      sigAudioASRError();
      return;
    }
    sigAudioASRError(); //stopRecorder
    videoStatus.value = 1;
    //聆听状态
  } else if (videoStatus.value == 3) {
    //思考状态
    //大模型回答播报状态
    stopRequest(); //logAiChatRecord
    videoStatus.value = 1;
  } else if (videoStatus.value == 4) {
    //聆听状态
    videoStatus.value = 1;
    stopPlayTextAudio();
  } else if (videoStatus.value == 1 || videoStatus.value == 9) {
    //休眠状态,异常结束
    videoStatus.value = 2;
    recording.value = false;
  }
};
/**数字形象自然语音状态切换 */
const handelModel = () => {
  if (videoStatus.value == 2) {
    AudioASRStreamState.value = true;
    if (question.value.trim().length === 0) {
      recording.value = false;
      Qrequest(chatQWeb.stopRecorder);
      stopCounter();
      router.push("/chat");
    } else {
      sigAudioASRError();
    }
  } else if (videoStatus.value == 3 || videoStatus.value == 4) {
    return;
  } else {
    stopCounter();
    router.push("/chat");
  }
};
/**监听麦克风状态 */
const sigAudioInputDevChange = (res) => {
  console.log("sigAudioInputDevChange");
  isAudioInput.value = res;
  if (!res) recording.value = false;
  if (!res) {
    videoStatus.value = 6;
  } else if (!isAudioOutput.value) {
    videoStatus.value = 10;
  } else {
    videoStatus.value = 1;
  }
};
/**监听输出设备状态 */
const sigAudioOutputDevChanged = async (res) => {
  isAudioOutput.value = res;
  if (!res) {
    playAudioID.value = "";
    if (videoStatus.value == 2 || videoStatus.value == 7) {
      await Qrequest(chatQWeb.stopRecorder);
    } else if (videoStatus.value == 3) {
      stopRequest(); //logAiChatRecord
    } else if (videoStatus.value == 4) {
      //聆听状态
      stopPlayTextAudio();
    }
    videoStatus.value = 10;
  } else if (!isAudioInput.value) {
    videoStatus.value = 6;
  } else {
    videoStatus.value = 1;
  }
};
/**快捷键开启录音 */
// const sigAudioRecShortcutPressed = () => {
//   console.log("sigAudioRecShortcutPressed")
//   activeFun();
// };

//语音播放
const playTextAudio = async (res) => {
  const { content, talkID } = _.last(history.value).anwsers[0];
  await Qrequest(chatQWeb.playTextAudio, talkID, res || content, true);
  playAudioID.value = talkID;
};
const stopPlayTextAudio = async () => {
  await Qrequest(chatQWeb.stopPlayTextAudio);
  playAudioID.value = "";
};

/**录音异常结束 */
const sigAudioASRError = async (err, text) => {
  console.log("sigAudioASRError", err, text);
  recording.value = false;
  await Qrequest(chatQWeb.stopRecorder);
  if (err && AudioASRStreamState.value) {
    router.push("/chat");
  }
  if (err && videoStatus.value !== 6) {
    videoStatus.value = 9;
    errorCon.value = text;
  }
};

// 语音播放结束
const sigPlayTTSFinished = (res) => {
  console.log("sigPlayTTSFinished", res);
  if (playAudioID.value === res) playAudioID.value = "";
  //上一条语音播放结束自动进入激活状态再去聆听
  if (
    videoStatus.value !== 5 &&
    videoStatus.value !== 6 &&
    videoStatus.value !== 8 &&
    videoStatus.value !== 9
  ) {
    videoStatus.value = 1;
  }
  showStop.value = false;
};
const sigPlayTTSError = (res) => (playAudioID.value = "");

//停止请求
const stopRequest = async () => {
  await Qrequest(chatQWeb.cancelAiRequest, talkID.value);
  const _question = history.value[history.value.length - 2].content;
  const { type, content } = _.last(_.last(history.value).anwsers);
  const { icon, displayname } = currentAccount.value;
  Qrequest(
    chatQWeb.logAiChatRecord,
    talkID.value,
    _question,
    content,
    false,
    0,
    "",
    type,
    icon ? icon : "",
    displayname ? displayname : ""
  );
  showStop.value = false;
  stopCounter();
};
// AI模型列表更新
const llmAccountLstChanged = (id, list) => {
  console.log(6555, id, list);
  const _list = JSON.parse(list);
  const index = _list.findIndex((item) => item.id === currentAccount.value.id);
  if (
    id !== currentAccount.value.id &&
    _list.length !== 0 &&
    videoStatus.value !== 5 &&
    videoStatus.value !== 6
  ) {
    videoStatus.value = 1;
  }
  if (!id) currentAccount.value = {};
  if (index > -1) {
    _list[index].active = true;
    currentAccount.value = _list[index];
  } else {
    _list.forEach((element) => {
      if (element.id === id) {
        element.active = true;
        currentAccount.value = element;
      }
    });
  }
  accountList.value = _list;
};

/**enter键,主动发送 */
const handleEnterKey = (event) => {
  console.log("handleEnterKey", question.value);
  if (question.value.trim().length !== 0) {
    sigAudioASRError();
  }
};
/**网络连接情况 */
const sigNetStateChanged = (res) => {
  if (!res) {
    if (videoStatus.value == 2) {
      sigAudioASRError();
    }
    if (videoStatus.value == 3) {
      stopRequest();
    }
    if (videoStatus.value == 4) {
      stopPlayTextAudio();
    }
    videoStatus.value = 5;
  } else {
    videoStatus.value = 1;
  }
};
// 接受AI图片信息
const sigText2PicFinish = (id, paths) => {
  _.last(_.last(history.value).anwsers).content = JSON.stringify(paths);
  const _question = history.value[history.value.length - 2].content;
  const _type = _.last(_.last(history.value).anwsers).type;
  const _status = _.last(_.last(history.value).anwsers).status;
  console.log(talkID.value, _question, paths, 0, "", _type, _status);
  const { icon, displayname } = currentAccount.value;
  Qrequest(
    chatQWeb.logAiChatRecord,
    talkID.value,
    _question,
    JSON.stringify(paths),
    false,
    0,
    "",
    _type,
    icon ? icon : "",
    displayname ? displayname : ""
  );
  showStop.value = false;
  if (_status >= 0 && _type === 2 && paths) {
    playTextAudio(
      useGlobalStore().loadTranslations[
        "The picture has been generated, please switch to the chat interface to view it."
      ]
    );
    if (videoStatus.value !== 6 && videoStatus.value !== 10) {
      videoStatus.value = 4;
    }
  }
};
watch(
  () => videoStatus.value,
  (newValue) => {
    onPlay.value = newValue;
    console.log("watch", newValue);
    if (newValue == 1) {
      chatQWeb &&
        chatQWeb.playSystemSound &&
        Qrequest(chatQWeb.playSystemSound, 1);
      stopPlayTextAudio();
      stopCounter();
      showStop.value = false;
    } else if (newValue == 2) {
      chatQWeb &&
        chatQWeb.playSystemSound &&
        Qrequest(chatQWeb.playSystemSound, 0);
      showStop.value = false;
      recording.value = false;
      handleRecorder();
      startCounter();
    } else {
      stopCounter();
    }
    if (newValue == 4 && ChatWindowState.value) {
      stopPlayTextAudio();
    }
    if (newValue == 5 || newValue == 6 || newValue == 8 || newValue == 10) {
      if (document.querySelector(".video5")) {
        document.querySelector(".video5").currentTime = 0;
        document.querySelector(".video5").play();
      }
      playStatus.value = true;
    } else if (newValue == 9) {
      if (document.querySelector(".video5")) {
        document.querySelector(".video5").currentTime = 0;
        document.querySelector(".video5").play();
      }
      playStatus.value = false;
    } else {
      document.querySelector(".video5") &&
        document.querySelector(".video5").pause();
      playStatus.value = false;
    }
  },
  { deep: true, immediate: false }
);
/** */
const sigChatConversationType = (id, type) => {
  _.last(_.last(history.value).anwsers).type = type;
};
/** 窗口关闭*/
const sigWebchat2BeHiden = () => {
  ChatWindowState.value = true;
  if (videoStatus.value == 2) {
    sigAudioASRError();
    videoStatus.value = 1;
  }
  if (videoStatus.value == 4) {
    stopPlayTextAudio();
  }
};
/** 窗口打开*/
const sigWebchat2BeShowed = async () => {
  ChatWindowState.value = false;
  if (videoStatus.value == 1) {
    videoStatus.value = 2;
  }
  if (videoStatus.value == 4) {
    sigPlayTTSFinished();
    videoStatus.value = 2;
  }
};
const sigThemeChanged = (res) => {
  updateTheme(res);
};
/** ctrl+super+c激活*/
const sigDigitalModeActive = () => {
  if (videoStatus.value == 1) {
    videoStatus.value = 2;
  }
};
// 活动色改变
const sigActiveColorChanged = (res) => updateActivityColor(res);

const handleActive = (res) =>{
    let maskDom = document.querySelector('#drop-mask');
    if (!maskDom) {
        maskDom = document.createElement("div");
        maskDom.id = "drop-mask"
        document.body.appendChild(maskDom);
    }
    maskDom.style.height = '100vh'
    maskDom.style.width = '100vw'
    maskDom.style.display = res ? 'flex' : 'none'
}
const sigWebchatModalityChanged = (res) => handleActive(res)

const responseAIFunObj = {
  sigAiReplyStream,
  sigAudioASRStream,
  sigAudioASRError,
  sigAudioCountDown,
  sigPlayTTSFinished,
  sigPlayTTSError,
  sigActiveColorChanged,
  sigAudioInputDevChange,
  sigAudioSampleLevel,
  sigNetStateChanged,
  sigText2PicFinish,
  sigChatConversationType,
  sigWebchat2BeHiden,
  sigWebchat2BeShowed,
  sigThemeChanged,
  llmAccountLstChanged,
  sigDigitalModeActive,
  sigAudioOutputDevChanged,
  sigWebchatModalityChanged
};
function handleKeyDown(event) {
  if (event.key === "Enter") {
    event.stopPropagation(); // 阻止事件冒泡
    // 或者
    event.preventDefault(); // 阻止默认行为
    // 然后触发你想要的方法
    handleEnterKey(event);
  }
}
onMounted(async () => {
  for (const key in responseAIFunObj) {
    if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
      chatQWeb[key].connect(responseAIFunObj[key]);
    }
  }
  document.addEventListener("keydown", handleKeyDown);
  initChat();
  useGlobalStore().loadTranslations = await Qrequest(chatQWeb.loadTranslations);
});
onBeforeUnmount(() => {
  for (const key in responseAIFunObj) {
    if (Object.hasOwnProperty.call(responseAIFunObj, key)) {
      chatQWeb[key].disconnect(responseAIFunObj[key]);
    }
  }
  document.removeEventListener("keydown", handleKeyDown);
});
</script>

<style lang="scss" scoped>
$scaleValue: var(--scale); // 缩放比例
$scaleValue1: var(--scale1); // 缩放比例
::selection {
  color: inherit;
}
video ::selection {
  background: none;
}
.main-content {
  position: relative;
  display: flex;
  flex-direction: column;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  color: var(--uosai-color-shortcut);
  font-family: "SourceHanSansSC-Medium";
  font-size: 16px;
  .logo {
    margin: 5px auto 0;
    text-align: center;
    width: 110px;
    img {
      width: 100%;
    }
    div {
      font-size: 20px;
      font-family: "Source Han Sans SC";
      font-weight: 700;
      margin-top: 7px;
    }
  }
  .close {
    width: 12px;
    height: 12px;
    position: absolute;
    right: 7px;
    top: 7px;
    padding: 12px;
    &:hover {
      cursor: pointer;
    }
  }
  .play {
    width: 36px;
    height: 36px;
    position: absolute;
    bottom: 7.86vh;
    left: 12.75vw;
    padding: 5px;
    fill: var(--uosai-color-clear);
    color: var(--uosai-color-clear);
    &:not(.disabledDig):hover {
      cursor: pointer;
      fill: var(--uosai-color-clear-hover);
      color: var(--uosai-color-clear-hover);
    }
    &:not(.disabledDig):active {
      fill: var(--activityColor);
      color: var(--activityColor);
    }
    z-index: 20;
    &.disabledDig {
      fill: var(--uosai-color-inputcontent-placeholder);
      color: var(--uosai-color-inputcontent-placeholder);
    }
  }
  .animate-box {
    position: absolute;
    width: 300px;
    height: 300px;
    text-align: center;
    z-index: 1;
    left: 50%;
    transform: translateX(-50%);
    top: 40%;
    &:hover {
      cursor: pointer;
    }
    .listen {
      position: absolute;
      display: flex;
      justify-content: center;
      width: 100%;
      height: auto;
      left: 0;
      img {
        width: 100%;
        height: auto;
      }
    }
    video {
      position: absolute;
      width: 100%;
      height: auto;
      z-index: 1;
      left: 0;
    }
  }
  .scale {
    position: absolute;
    width: 300px;
    left: 0;
    animation: scale 50ms linear;
    animation-fill-mode: forwards;
  }
  .scale1 {
    position: absolute;
    width: 300px;
    left: 0;
    animation: scale1 50ms linear;
    animation-fill-mode: forwards;
    // animation: scale1 50ms ease-in-out infinite;
  }
  .rotate {
    transform-origin: center center; /* 设置旋转中心为圆心 */
    animation: rotate 6s linear infinite;
  }
  .active-box {
    width: 100%;
    margin: 0 auto 6.12vh;
    left: 0;
    text-align: center;
    font-family: "Source Han Sans SC";
    font-size: 20px;
    z-index: 10;
    .pr {
      position: relative;
    }
    .active-con {
      .dian {
        letter-spacing: 1.5px;
        margin-left: 0px;
        display: inline-block;
      }
      pre {
        word-break: break-all;
        white-space: pre-wrap;
        line-height: 25px;
        width: 80vw;
        margin: auto;
        font-size: 14px;
      }
      .go-config {
        font-size: 14px;
        color: var(--activityColor);
        cursor: pointer;
        text-decoration: none;
      }
      svg {
        width: 10px;
        height: 14px;
        margin-right: 8px;
      }
      .fz-14 {
        font-size: 14px;
        margin-top: 5px;
      }
    }
    .handel {
      position: relative;
      width: 70px;
      height: 70px;
      .svgBg {
        width: 70px;
        height: 70px;
        background: var(--activityColor);
        border-radius: 50%;
        display: flex;
        justify-content: center;
        align-items: center;
      }
      .svgBg-hover {
        width: 70px;
        height: 70px;
        position: absolute;
        top: 0;
        background: rgba(255, 255, 255, 0.1);
        border-radius: 50%;
        display: none;
      }
      .icon-dig-play {
        width: 18px;
        height: 20px;
      }
      .icon-dig-stop {
        width: 16px;
        height: 16px;
      }
      margin: 14.6vh auto 0;
    }
  }
  .disabledDig {
    cursor: not-allowed;
  }
  :deep(.icon-change-normal) {
    fill: var(--activityColor) !important;
  }
}
@keyframes scale {
  0% {
    transform: scale(1); /*开始为原始大小*/
  }
  100% {
    transform: scale($scaleValue);
  }
}
@keyframes scale1 {
  0% {
    transform: scale(1); /*开始为原始大小*/
  }
  100% {
    transform: scale($scaleValue1);
  }
  // 100% {
  //   transform: scale(1);
  // }
}
@keyframes rotate {
  0% {
    transform: rotate(0deg);
  }
  50% {
    transform: rotate(180deg);
  }
  100% {
    transform: rotate(360deg);
  }
}
.handel:not(.disabledDig):hover {
  cursor: pointer;
}
.handel:not(.disabledDig):hover > .svgBg-hover {
  display: block;
}
.handel:not(.disabledDig):active > .svgBg-hover {
  background: rgba(255, 255, 255, 0.05);
  display: block;
}
.handel:not(.disabledDig):active .svg-icon {
  opacity: 0.6;
}
.handel.disabledDig {
  .svgBg {
    opacity: 0.4;
  }
}
.dark {
  .handel:not(.disabledDig):hover > .svg-icon {
    display: block;
    background: rgba(255, 255, 255, 0.1);
  }
  .handel:not(.disabledDig):active > .svgBg-hover {
    background: rgba(255, 255, 255, 0.05);
    display: block;
  }
}
</style>
