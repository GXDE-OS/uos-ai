import { createRouter, createWebHashHistory } from 'vue-router'
const router = createRouter({
  history: createWebHashHistory(),
  routes: [
    {
      path: '/',
      name: 'Root',
      component: () => import('@/views/root/Root.vue'),
    },
    {
      path: '/app',
      name: 'App',
      component: () => import('@/views/window/RootWindow'),
    },
  ],
})

export default router
