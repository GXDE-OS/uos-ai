import { createApp } from "vue";
import { createPinia } from "pinia";
import App from "./App.vue";
import Router from "./router/index";
import mitt from "mitt";
import "./style.css";
import 'element-plus/dist/index.css'
import "./theme/theme.css";
import SvgIcon from "./components/svgIcon/svgIcon.vue";


const Mitt = mitt();
const app = createApp(App);
//挂载全局event事件对象
app.config.globalProperties.$Bus = Mitt;
app.component("svgIcon", SvgIcon);
app.use(createPinia()).use(Router).mount("#app");
