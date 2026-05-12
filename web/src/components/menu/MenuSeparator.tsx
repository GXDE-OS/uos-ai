import { defineComponent } from "vue";
import "@/assets/styles/components/menu/MenuSeparator.css";

export default defineComponent({
    name: "MenuSeparator",
    render() {
        return (
            <div class="menu-separator">
                <div class="menu-separator__line"></div>
            </div>
        );
    },
});
