import { defineComponent, nextTick, ref, watch } from "vue";
import type { PropType } from "vue";

export default defineComponent({
    name: "OutlineTitle",

    props: {
        // 大纲标题
        title: {
            type: String,
            required: true,
        },
        // 是否可编辑
        editable: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        // 标题更新
        updateTitle: (value: string) => typeof value === "string",
    },

    setup(props, { emit }) {
        const isEditing = ref(false);
        const draftTitle = ref(props.title);
        const inputRef = ref<HTMLInputElement | null>(null);

        watch(
            () => props.title,
            (value) => {
                if (!isEditing.value) {
                    draftTitle.value = value;
                }
            },
        );

        const enterEdit = () => {
            if (!props.editable) return;
            isEditing.value = true;
            draftTitle.value = props.title;
            nextTick(() => {
                inputRef.value?.focus();
                inputRef.value?.select();
            });
        };

        const commitEdit = () => {
            if (!isEditing.value) return;
            const nextTitle = draftTitle.value.trim();

            // 清空标题视为放弃编辑，恢复原标题
            if (!nextTitle) {
                isEditing.value = false;
                draftTitle.value = props.title;
                return;
            }

            isEditing.value = false;
            emit("updateTitle", draftTitle.value);
        };

        const cancelEdit = () => {
            if (!isEditing.value) return;
            isEditing.value = false;
            draftTitle.value = props.title;
        };

        const handleKeyDown = (event: KeyboardEvent) => {
            if (event.key === "Enter") {
                event.preventDefault();
                commitEdit();
                return;
            }

            if (event.key === "Escape") {
                event.preventDefault();
                cancelEdit();
            }
        };

        return {
            isEditing,
            draftTitle,
            inputRef,
            enterEdit,
            commitEdit,
            handleKeyDown,
        };
    },

    render() {
        return (
            <div class={{ 'outline-title': true, 'outline-title--input-focused': this.isEditing }}>
                {this.isEditing ? (
                    <input
                        ref="inputRef"
                        class="outline-title__input"
                        value={this.draftTitle}
                        onInput={(event) => {
                            this.draftTitle = (event.target as HTMLInputElement).value;
                        }}
                        onBlur={this.commitEdit}
                        onKeydown={this.handleKeyDown}
                    />
                ) : (
                    <div class="outline-title__text" onClick={this.enterEdit}>
                        {this.$props.title}
                    </div>
                )}
            </div>
        );
    },
});
