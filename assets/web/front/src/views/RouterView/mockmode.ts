import { useRouter } from "vue-router";
import { useGlobalStore } from "../../store/global";
import WebSocketChannel from '../../common/WebSocketChannel.js';

var initialize = async function() {
    const router = useRouter();
    const store = useGlobalStore();
    const wsChannel = new WebSocketChannel('ws://localhost:8081', qt.webChannelTransport)
    wsChannel.connect().then(() => {
        router.replace({ name: "Chat" });
    })
}

export default initialize