import Qt from "../../common/QWebChannel";
import WebSocketTransport from '../../common/WebSocketTransport.js';
import { useRouter } from "vue-router";
import { useGlobalStore } from "../../store/global";

var initialize = async function() {
    const router = useRouter();
    const store = useGlobalStore();

    const wsTransport = new WebSocketTransport('ws://localhost:8081');
    wsTransport.connect().then(() => {
        new Qt(wsTransport, function (channel) {
        store.chatQWeb = channel.objects.chatObject;
        store.updateActivityColor(store.chatQWeb.activeColor);
        store.updateTheme(store.chatQWeb.themeType);
        router.replace({ name: "Chat" });
        });
    })
}

export default initialize