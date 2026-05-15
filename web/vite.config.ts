import { fileURLToPath, URL } from "node:url";

import { defineConfig } from "vite";
import vue from "@vitejs/plugin-vue";
import vueJsx from "@vitejs/plugin-vue-jsx";
import legacy from "@vitejs/plugin-legacy";
import vueDevTools from "vite-plugin-vue-devtools";
import { svgBuilder } from "./src/common/svgBuilder";

// https://vite.dev/config/
export default defineConfig({
    base: "./",
    plugins: [
        vue(),
        vueJsx(),
        vueDevTools(),
        legacy({
            targets: ["chrome 65", "not IE 11"], //打包降级 兼容到chrome65内核  es6+ -> es5
        }),
        svgBuilder(fileURLToPath(new URL("./src/assets/icons/", import.meta.url))),
    ],
    resolve: {
        alias: {
            "@": fileURLToPath(new URL("./src", import.meta.url)),
            "@views": fileURLToPath(new URL("./src/views", import.meta.url)),
            "@components": fileURLToPath(
                new URL("./src/components", import.meta.url),
            ),
        },
    },
    build: {
        target: ["chrome65"],
        cssTarget: ["chrome65"],
        manifest: false,
        outDir: "dist",
        assetsDir: "assets",
        minify: "terser",
        terserOptions: {
            //去除console 和 debugger
            compress: {
                // drop_console: true,
                drop_debugger: true,
            },
        },
        rollupOptions: {
            output: {
                entryFileNames: `assets/[name].js`,
                chunkFileNames: `assets/[name].js`,
                assetFileNames: `assets/[name].[ext]`
            },
        },
    },
});
