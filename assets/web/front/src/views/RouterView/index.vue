<template></template>
<script setup>
// 后端调试模式，编译进Qt
//import initialize from './mockmode.ts';
// 前端调试模式
//import initialize from './devfront.ts';
// 产品模式
import initialize from './prodmode.ts';

initialize();

</script>