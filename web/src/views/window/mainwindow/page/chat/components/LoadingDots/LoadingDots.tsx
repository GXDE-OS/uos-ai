import { defineComponent } from "vue";

export default defineComponent({
    name: "LoadingDots",
    
    setup() {
        return {};
    },

    render() {
        return (
            <div class="loading-container">
                <div class="loading-dot"></div>
                <div class="loading-dot"></div>
                <div class="loading-dot"></div>
            </div>
        );
    },
});