import { defineComponent, nextTick, ref, watch, TransitionGroup, computed, inject } from "vue";
import type { PropType } from "vue";
import type { OutlineParagraph, OutlineSection as OutlineSubsectionData } from "@/types/conversation";
import { useNotifyStore, useBackendStore } from "@/stores";
import OutlineSubsection from "./OutlineSubsection";
import SvgIcon from "@/components/SvgIcon";
import Tooltip from "@/components/Tooltip";

/** 调整 textarea 高度跟随内容自适应 */
const adjustHeight = (el: HTMLTextAreaElement) => {
    el.style.height = "auto";
    el.style.height = `${el.scrollHeight}px`;
};

export default defineComponent({
    name: "OutlineSection",

    components: {
        OutlineSubsection,
        SvgIcon,
    },

    props: {
        // 大章节数据
        paragraph: {
            type: Object as PropType<OutlineParagraph>,
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
        // 删除大章节事件
        delete: (index: number) => typeof index === "number",
        // 删除小章节事件
        deleteSubsection: (paragraphIndex: number, subsectionIndex: number) =>
            typeof paragraphIndex === "number" && typeof subsectionIndex === "number",
        // 更新章节标题
        updateTitle: (index: number, value: string) => typeof index === "number" && typeof value === "string",
        // 更新小章节标题
        updateSubsectionTitle: (paragraphIndex: number, subsectionIndex: number, value: string) =>
            typeof paragraphIndex === "number" && typeof subsectionIndex === "number" && typeof value === "string",
        // 新增子章节
        addSubsection: (paragraphIndex: number, title: string) =>
            typeof paragraphIndex === "number" && typeof title === "string",
        // 拖拽开始
        dragStart: (event: MouseEvent) => event instanceof MouseEvent,
        // 子章节重新排序
        reorderSubsections: (paragraphIndex: number) => typeof paragraphIndex === "number",
    },

    setup(props, { emit }) {
        const notifyStore = useNotifyStore();
        const backend = useBackendStore();
        const isEditing = ref(false);
        const draftTitle = ref(props.paragraph.title);
        const inputRef = ref<HTMLTextAreaElement | null>(null);
        const subsectionKeyMap = new WeakMap<OutlineSubsectionData, string>();
        let subsectionKeySeed = 0;

        // ── 新增子章节状态 ──
        const isAdding = ref(false);
        const newTitle = ref("");
        const addInputRef = ref<HTMLInputElement | null>(null);
        const skipAddBlur = ref(false);

        // ── 子章节 hover 状态 ──
        const isSubsectionHovered = ref(false);

        // 注入全局拖拽状态设置方法
        const setOutlineGlobalDragging = inject<((dragging: boolean) => void) | null>(
            "setOutlineGlobalDragging",
            null,
        );

        // 国际化文案
        const deleteTitleText = computed(() => backend.translate("Delete this heading?"));
        const cancelText = computed(() => backend.translate("Cancel"));
        const deleteText = computed(() => backend.translate("Delete"));
        const enterSubsectionTitleText = computed(() => backend.translate("Enter chapter title"));

        const getSubsectionKey = (subsection: OutlineSubsectionData) => {
            const cachedKey = subsectionKeyMap.get(subsection);
            if (cachedKey) return cachedKey;

            subsectionKeySeed += 1;
            const nextKey = `${props.domKey}-sub-${subsectionKeySeed}`;
            subsectionKeyMap.set(subsection, nextKey);
            return nextKey;
        };

        const settleSubsectionGhost = async (ghost: HTMLElement, subsectionKey: string, onSettled?: () => void) => {
            await nextTick();

            const target = document.querySelector<HTMLElement>(
                `[data-outline-paragraph-key="${props.domKey}"] [data-outline-subsection-key="${subsectionKey}"]`,
            );
            if (!target) {
                onSettled?.();
                ghost.remove();
                return;
            }

            const rect = target.getBoundingClientRect();
            ghost.classList.add("outline-subsection--ghost-settling");

            await new Promise<void>((resolve) => {
                const finish = () => {
                    ghost.removeEventListener("transitionend", handleTransitionEnd);
                    resolve();
                };

                const handleTransitionEnd = (event: TransitionEvent) => {
                    if (event.target !== ghost) return;
                    finish();
                };

                ghost.addEventListener("transitionend", handleTransitionEnd);
                requestAnimationFrame(() => {
                    ghost.style.top = `${rect.top + window.scrollY}px`;
                    ghost.style.left = `${rect.left}px`;
                    ghost.style.width = `${rect.width}px`;
                    ghost.style.height = `${rect.height}px`;
                });
                window.setTimeout(finish, 220);
            });

            onSettled?.();

            ghost.style.opacity = "0";
            await new Promise<void>((resolve) => {
                window.setTimeout(resolve, 120);
            });

            ghost.remove();
        };

        const moveSubsection = (from: number, to: number) => {
            if (from === to) return;
            const content = props.paragraph.content;
            const [item] = content.splice(from, 1);
            if (!item) return;
            content.splice(to, 0, item);
        };

        watch(
            () => props.paragraph.title,
            (value) => {
                if (!isEditing.value) {
                    draftTitle.value = value;
                }
            },
        );

        // 处理删除按钮点击
        const handleDelete = () => {
            if (!props.editable) return;
            void notifyStore
                .showDialog({
                    title: deleteTitleText.value,
                    buttons: [
                        { key: "cancel", text: cancelText.value, type: "default" },
                        { key: "confirm", text: deleteText.value, type: "danger", suggested: true },
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
            draftTitle.value = props.paragraph.title;
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
                draftTitle.value = props.paragraph.title;
                return;
            }

            isEditing.value = false;
            emit("updateTitle", props.index, draftTitle.value);
        };

        const cancelEdit = () => {
            if (!isEditing.value) return;
            isEditing.value = false;
            draftTitle.value = props.paragraph.title;
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

        // 处理小章节删除
        const handleSubsectionDelete = (subsectionIndex: number) => {
            emit("deleteSubsection", props.index, subsectionIndex);
        };

        const handleSubsectionTitleUpdate = (subsectionIndex: number, value: string) => {
            emit("updateSubsectionTitle", props.index, subsectionIndex, value);
        };

        // ── 新增子章节操作 ──
        const handleAddClick = () => {
            if (!props.editable) return;
            isAdding.value = true;
            newTitle.value = "";
            nextTick(() => {
                addInputRef.value?.focus();
            });
        };

        const commitAdd = () => {
            const title = newTitle.value.trim();
            isAdding.value = false;
            newTitle.value = "";
            if (!title) return; // 空内容视为放弃
            emit("addSubsection", props.index, title);
        };

        const cancelAdd = () => {
            isAdding.value = false;
            newTitle.value = "";
        };

        const handleAddBlur = () => {
            if (skipAddBlur.value) {
                skipAddBlur.value = false;
                return;
            }
            commitAdd();
        };

        const handleAddKeyDown = (event: KeyboardEvent) => {
            if (event.key === "Enter") {
                event.preventDefault();
                skipAddBlur.value = true;
                commitAdd();
                return;
            }
            if (event.key === "Escape") {
                event.preventDefault();
                skipAddBlur.value = true;
                cancelAdd();
                addInputRef.value?.blur();
            }
        };

        // 拖拽：整个 section 头部区域均可触发，排除标题文字、按钮、输入框、子章节区域
        const handleSectionDragMouseDown = (event: MouseEvent) => {
            if (!props.editable) return;
            // 自身标题编辑或新增子章节时禁止拖拽
            if (isEditing.value || isAdding.value) return;
            // 任意子章节处于编辑状态时也禁止拖拽
            const sectionEl = event.currentTarget as HTMLElement;
            if (sectionEl.querySelector(".outline-subsection--input-focused")) return;
            const target = event.target as HTMLElement;
            if (
                target.closest(
                    "button, input, textarea, .outline-section__button, .outline-section__title-text, .outline-section__content",
                )
            )
                return;
            emit("dragStart", event);
        };

        // 子章节拖拽排序
        const subsectionDragState = ref<{
            draggingSubsection: OutlineSubsectionData;
            draggingKey: string;
            currentIndex: number;
            moved: boolean;
            ghost: HTMLElement | null;
            offsetY: number;
        } | null>(null);

        const isSubsectionDragging = (subsection: OutlineSubsectionData) => {
            return subsectionDragState.value?.draggingSubsection === subsection;
        };

        const handleSubsectionDragStart = (event: MouseEvent, index: number) => {
            if (!props.editable) return;
            // 编辑或新增状态下禁止拖拽
            if (isEditing.value || isAdding.value) return;
            event.preventDefault();

            const target = event.currentTarget as HTMLElement;
            const subsEl = target.closest(".outline-subsection") as HTMLElement | null;
            if (!subsEl) return;

            const rect = subsEl.getBoundingClientRect();
            const ghost = subsEl.cloneNode(true) as HTMLElement;
            ghost.classList.add("outline-subsection--ghost");
            ghost.style.width = `${rect.width}px`;
            ghost.style.height = `${rect.height}px`;
            ghost.style.top = `${rect.top + window.scrollY}px`;
            ghost.style.left = `${rect.left}px`;
            // ghost 挂到 body，确保 position:fixed 以 viewport 为基准定位
            // （outline-section 含 will-change:transform，会建立新 stacking context，
            //   若挂在其内部子集，fixed 定位可能退化为相对该元素，导致坐标偏移）
            document.body.appendChild(ghost);

            subsectionDragState.value = {
                draggingSubsection: props.paragraph.content[index] as OutlineSubsectionData,
                draggingKey: getSubsectionKey(props.paragraph.content[index] as OutlineSubsectionData),
                currentIndex: index,
                moved: false,
                ghost,
                offsetY: event.clientY - rect.top,
            };

            // 通知父组件更新全局拖拽状态
            setOutlineGlobalDragging?.(true);

            const onMouseMove = (e: MouseEvent) => {
                const state = subsectionDragState.value;
                if (!state || !state.ghost) return;

                state.ghost.style.top = `${e.clientY - state.offsetY + window.scrollY}px`;

                const container = subsEl.closest(".outline-section__content");
                if (!container) return;
                const items = Array.from(container.querySelectorAll<HTMLElement>(".outline-subsection"));

                let newInsert = items.length;
                for (let i = 0; i < items.length; i++) {
                    if (i === state.currentIndex) continue;
                    const el = items[i];
                    if (!el) continue;
                    const r = el.getBoundingClientRect();
                    if (e.clientY < r.top + r.height / 2) {
                        newInsert = i;
                        break;
                    }
                }
                const targetIndex = newInsert > state.currentIndex ? newInsert - 1 : newInsert;
                if (targetIndex !== state.currentIndex) {
                    moveSubsection(state.currentIndex, targetIndex);
                    state.currentIndex = targetIndex;
                    state.moved = true;
                }
            };

            const onMouseUp = async () => {
                document.removeEventListener("mousemove", onMouseMove);
                document.removeEventListener("mouseup", onMouseUp);

                const state = subsectionDragState.value;
                if (!state) return;

                if (state.moved) {
                    if (state.ghost) {
                        await settleSubsectionGhost(state.ghost, state.draggingKey, () => {
                            subsectionDragState.value = null;
                            setOutlineGlobalDragging?.(false);
                        });
                    } else {
                        subsectionDragState.value = null;
                        setOutlineGlobalDragging?.(false);
                    }

                    // ghost 收尾结束后再触发回灌，避免同帧重渲染造成闪烁。
                    emit("reorderSubsections", props.index);
                    return;
                }

                if (state.ghost) state.ghost.remove();
                subsectionDragState.value = null;
                setOutlineGlobalDragging?.(false);
            };

            document.addEventListener("mousemove", onMouseMove);
            document.addEventListener("mouseup", onMouseUp);
        };

        return {
            isEditing,
            draftTitle,
            inputRef,
            handleDelete,
            enterEdit,
            commitEdit,
            handleKeyDown,
            handleSubsectionDelete,
            handleSubsectionTitleUpdate,
            isAdding,
            newTitle,
            addInputRef,
            handleAddClick,
            handleAddBlur,
            handleAddKeyDown,
            handleSectionDragMouseDown,
            handleSubsectionDragStart,
            enterSubsectionTitleText,
            getSubsectionKey,
            isSubsectionDragging,
            isSubsectionHovered,
            subsectionDragState,
        };
    },

    render() {
        return (
            <div
                data-outline-paragraph-key={this.$props.domKey}
                class={{
                    "outline-section": true,
                    "outline-section--input-focused": this.isEditing,
                    "outline-section--editable": this.$props.editable,
                    "outline-section--drag-placeholder": this.$props.dragging,
                    "outline-section--subsection-hover": this.isSubsectionHovered,
                }}
                onMousedown={this.handleSectionDragMouseDown}
            >
                <div class="outline-section__header">
                    {this.isEditing ? (
                        <textarea
                            ref="inputRef"
                            class="outline-section__input"
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
                        <div class="outline-section__title">
                            <span class="outline-section__title-text" onClick={this.enterEdit}>
                                {this.$props.paragraph.title}
                            </span>
                        </div>
                    )}
                    <Tooltip content={useBackendStore().translate("Add sub-chapter")} placement="top">
                        <div
                            class={{ "outline-section__button": true, disabled: !this.$props.editable }}
                            onClick={this.handleAddClick}
                        >
                            <SvgIcon icon="add" />
                        </div>
                    </Tooltip>
                    <Tooltip content={useBackendStore().translate("Delete chapter")} placement="top">
                        <div
                            class={{ "outline-section__button": true, disabled: !this.$props.editable }}
                            onClick={this.handleDelete}
                        >
                            <SvgIcon icon="trash" />
                        </div>
                    </Tooltip>
                </div>
                <div
                    class={{
                        "outline-section__content": true,
                        "outline-section__content--no-adding": !this.isAdding,
                    }}
                >
                    <TransitionGroup name="subsection-list" tag="div">
                        {this.$props.paragraph.content.map((subsection, index) => (
                            <div
                                key={this.getSubsectionKey(subsection)}
                                class="outline-subsection-wrapper"
                                onMouseenter={() => {
                                    // 拖拽过程中不更新 hover 状态，避免干扰动画
                                    if (this.subsectionDragState) return;
                                    this.isSubsectionHovered = true;
                                }}
                                onMouseleave={() => {
                                    if (this.subsectionDragState) return;
                                    this.isSubsectionHovered = false;
                                }}
                            >
                                <OutlineSubsection
                                    domKey={this.getSubsectionKey(subsection)}
                                    section={subsection}
                                    editable={this.$props.editable}
                                    dragging={this.isSubsectionDragging(subsection)}
                                    index={index}
                                    onDelete={this.handleSubsectionDelete}
                                    onUpdateTitle={this.handleSubsectionTitleUpdate}
                                    onDragStart={(e: MouseEvent) => this.handleSubsectionDragStart(e, index)}
                                />
                            </div>
                        ))}
                    </TransitionGroup>
                    {this.isAdding && (
                        <div class="outline-subsection--adding">
                            <input
                                ref="addInputRef"
                                class="outline-subsection__add-input"
                                value={this.newTitle}
                                placeholder={this.enterSubsectionTitleText}
                                onInput={(event) => {
                                    this.newTitle = (event.target as HTMLInputElement).value;
                                }}
                                onBlur={this.handleAddBlur}
                                onKeydown={this.handleAddKeyDown}
                            />
                        </div>
                    )}
                </div>
            </div>
        );
    },
});
