import { defineComponent, nextTick, ref, watch } from "vue";
import type { PropType } from "vue";
import type { OutlineSection } from "@/types/conversation";
import { useBackendStore, useNotifyStore } from "@/stores";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";

/** 调整 textarea 高度跟随内容自适应 */
const adjustHeight = (el: HTMLTextAreaElement) => {
    el.style.height = "auto";
    el.style.height = `${el.scrollHeight}px`;
};

export default defineComponent({
    name: "OutlineSubsection",

    components: { SvgIcon },

    props: {
        // 小章节数据
        section: {
            type: Object as PropType<OutlineSection>,
            required: true,
        },
        // 是否可编辑
        editable: {
            type: Boolean,
            default: false,
        },
        // 章节索引
        index: {
            type: Number,
            default: 0,
        },
        domKey: {
            type: String,
            required: true,
        },
        dragging: {
            type: Boolean,
            default: false,
        },
    },

    emits: {
        // 删除事件
        delete: (index: number) => typeof index === "number",
        // 更新标题
        updateTitle: (index: number, value: string) => typeof index === "number" && typeof value === "string",
        // 拖拽开始
        dragStart: (event: MouseEvent) => event instanceof MouseEvent,
    },

    setup(props, { emit }) {
        const notifyStore = useNotifyStore();
        const isEditing = ref(false);
        const draftTitle = ref(props.section.title);
        const inputRef = ref<HTMLTextAreaElement | null>(null);

        watch(
            () => props.section.title,
            (value) => {
                if (!isEditing.value) {
                    draftTitle.value = value;
                }
            },
        );

        // 处理删除按钮点击
        const handleDelete = () => {
            if (!props.editable) return;
            const bc = useBackendStore();
            void notifyStore
                .showDialog({
                    title: bc.translate("Delete this heading?"),
                    buttons: [
                        { key: "cancel", text: bc.translate("Cancel"), type: "default" },
                        { key: "confirm", text: bc.translate("Delete"), type: "danger", suggested: true },
                    ],
                })
                .then((result) => {
                    if (result.key === "confirm") {
                        emit("delete", props.index);
                    }
                });
        };

        const enterEdit = () => {
            if (!props.editable) return;
            isEditing.value = true;
            draftTitle.value = props.section.title;
            nextTick(() => {
                if (inputRef.value) {
                    inputRef.value.focus();
                    inputRef.value.select();
                    adjustHeight(inputRef.value);
                }
            });
        };

        const commitEdit = () => {
            if (!isEditing.value) return;
            const nextTitle = draftTitle.value.trim();

            // 清空标题视为放弃编辑，恢复原标题
            if (!nextTitle) {
                isEditing.value = false;
                draftTitle.value = props.section.title;
                return;
            }

            isEditing.value = false;
            emit("updateTitle", props.index, draftTitle.value);
        };

        const cancelEdit = () => {
            if (!isEditing.value) return;
            isEditing.value = false;
            draftTitle.value = props.section.title;
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

        const handleDragMouseDown = (event: MouseEvent) => {
            if (!props.editable) return;
            // 编辑状态下禁止拖拽
            if (isEditing.value) return;
            emit("dragStart", event);
        };

        return {
            isEditing,
            draftTitle,
            inputRef,
            handleDelete,
            enterEdit,
            commitEdit,
            handleKeyDown,
            handleDragMouseDown,
        };
    },

    render() {
        return (
            <div
                class={{
                    "outline-subsection": true,
                    "outline-subsection--input-focused": this.isEditing,
                    "outline-subsection--drag-placeholder": this.$props.dragging,
                }}
                data-outline-subsection-key={this.$props.domKey}
            >
                <span
                    class={{ "outline-drag-handle": true, disabled: !this.$props.editable }}
                    onMousedown={this.handleDragMouseDown}
                >
                    <SvgIcon icon="drag-handle" />
                </span>
                <div class="outline-subsection__content">
                    <div class="outline-subsection__title">
                        {this.isEditing ? (
                            <textarea
                                ref="inputRef"
                                class="outline-subsection__input"
                                value={this.draftTitle}
                                rows={1}
                                maxlength={500}
                                onInput={(event) => {
                                    this.draftTitle = (event.target as HTMLTextAreaElement).value;
                                    adjustHeight(event.target as HTMLTextAreaElement);
                                }}
                                onBlur={this.commitEdit}
                                onKeydown={this.handleKeyDown}
                            />
                        ) : (
                            <span class="outline-subsection__title-text" onClick={this.enterEdit}>
                                {this.$props.section.title}
                            </span>
                        )}
                    </div>
                    {this.$props.editable && (
                        <Tooltip content={useBackendStore().translate("Delete sub-chapter")} placement="top">
                            <div class="outline-section__button outline-subsection__delete" onClick={this.handleDelete}>
                                <SvgIcon icon="trash" size={[16, 16]} />
                            </div>
                        </Tooltip>
                    )}
                </div>
            </div>
        );
    },
});
