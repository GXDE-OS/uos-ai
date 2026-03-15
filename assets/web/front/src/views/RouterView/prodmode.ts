import Qt from "../../common/QWebChannel";
import { useRouter } from "vue-router";
import { useGlobalStore } from "../../store/global";

var initialize = async function() {
    const router = useRouter();
    const store = useGlobalStore();

    new Qt(qt.webChannelTransport, function (channel) {
        store.chatQWeb = channel.objects.chatObject;
        store.updateActivityColor(store.chatQWeb.activeColor);
        store.updateTheme(store.chatQWeb.themeType)
        router.replace({ name: "Chat" });
      });
}

export default initialize