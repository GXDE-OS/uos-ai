import { createRouter, createWebHashHistory } from "vue-router";

const routes = [
  {
    path: "/",
    name: "RouterView",
    component: () => import("../views/RouterView/index.vue"),
  },
  {
    path: "/chat",
    name: "Chat",
    component: () => import("../views/chat/index.vue"),
  },  
  {  
    path: "/DigitalImage",  
    name: "DigitalImage",
    component: () => import("../views/chat/components/DigitalImage.vue"),  
  }
];

const router = createRouter({
  history: createWebHashHistory(),
  routes,
});

export default router;
