import { defineComponent } from "vue";

import { useBackendStore } from "@/stores";
import { WindowMode } from "@/types/windowinfo";

export default defineComponent({
    name: "MiniWindow",
    components: {},
    setup() {
        const backend = useBackendStore();
        // test
        const switchToSideWindow = () => {
            backend.requestWindow("switchMode", WindowMode.Side);
        };

        return {
            switchToSideWindow,
        };
    },
    render() {
        return (
            <div class="mini-window">
                <div class="mini-content">
                    <div class="content-body">
                        <h3>MiniWindow</h3>
                        <p>这是MiniWindow组件的内容区域</p>
                        <button onClick={this.switchToSideWindow}>
                            {" "}
                            切换到侧边窗口{" "}
                        </button>
                    </div>
                </div>
            </div>
        );
    },
});
