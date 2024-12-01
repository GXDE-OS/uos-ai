<template>
  <svg v-bind="$attrs" :class="svgClass" :style="{ color }" aria-hidden="true">
    <use :xlink:href="iconName"></use>
  </svg>
</template>

<script setup>
import { computed } from "vue";
const props = defineProps({
  icon: {
    type: String,
    required: true,
  },
  color: {
    type: String,
  },
  size: {
    type: Array,
  },
});
const iconName = computed(() => {
  // console.log("iconName: ", `#icon-${props.icon}`)
  return `#icon-${props.icon}`;
});
const svgClass = computed(() => {
  if (props.icon) {
    return `svg-icon icon-${props.icon}`;
  }
  return `svg-icon`;
});
</script>

<style lang="scss" scoped>
.svg-icon {
  width: 1em;
  height: 1em;
  fill: currentColor;
  overflow: hidden;
}
</style>
