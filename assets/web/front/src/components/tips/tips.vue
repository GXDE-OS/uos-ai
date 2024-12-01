<template>
  <div class="uos-tips" :tip="tip" :class="className">
    <div class="tips-con">{{ tip }}</div>
  </div>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({
  tip: {
    type: String,
    required: true,
  },
  className: {
    type: Boolean,
  },
});
</script>

<style lang="scss" scoped>
.uos-tips {
  position: absolute;
  font-size: 0.85rem;
  padding: 4px 5px;
  opacity: 1;
  background: #fafafa;
  border: 1px solid rgba(0, 0, 0, 0.05);
  box-shadow: 0px 6px 20px 0px rgba(0, 0, 0, 0.2);
  border-radius: 8px;
  color: #000;
  z-index: 10;
  width: max-content;
}
.top-right {
  top: 3px;
  left: 50px;
}
.bot-left {
  bottom: -19px;
  right: 25px;
}
.dark .uos-tips {
  background-color: #2c2c2c;
  color: #C0C6D4;
  border: 1px solid rgba(0, 0, 0, 0.3);
}
</style>
