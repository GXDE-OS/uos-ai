import { useRouter } from "vue-router";
import WebSocketChannel from './WebSocketChannel';
import { useBackendStore } from "@/stores";

var initialize = async function() {
    const router = useRouter()
    const back = useBackendStore()
    const wsChannel = new WebSocketChannel('ws://localhost:8081', qt.webChannelTransport)
    wsChannel.connect().then(() => {
        router.replace({ name: "App" })
    })
}

export default initialize