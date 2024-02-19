import { directiveHooks } from '@vueuse/shared'
import { onClickOutside } from '@vueuse/core'

export const vOnClickOutside = {
    [directiveHooks.mounted](el, binding) {
        const capture = !binding.modifiers.bubble
        if (typeof binding.value === 'function') {
            el.__onClickOutside_stop = onClickOutside(el, binding.value, { capture })
        }
        else {
            const [handler, options] = binding.value
                ; el.__onClickOutside_stop = onClickOutside(el, handler, Object.assign({ capture }, options))
        }
    },
    [directiveHooks.unmounted](el) {
        el.__onClickOutside_stop()
    },
}

// alias
export { vOnClickOutside as VOnClickOutside }