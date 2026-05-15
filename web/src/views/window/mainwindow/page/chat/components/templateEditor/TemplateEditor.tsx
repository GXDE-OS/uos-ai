import { defineComponent, markRaw, nextTick, onBeforeUnmount, onMounted, ref, shallowRef, watch } from "vue";
import { Schema, Fragment, Slice, type Node as PMNode } from "prosemirror-model";
import ScrollBar from "@/components/ScrollBar";
import { EditorState, Plugin, Selection } from "prosemirror-state";
import { EditorView, type NodeView } from "prosemirror-view";
import { history, redo, undo } from "prosemirror-history";
import { keymap } from "prosemirror-keymap";
import { baseKeymap } from "prosemirror-commands";
import { parseTemplate } from "./templateParser";

const schema = new Schema({
    nodes: {
        doc: {
            content: "paragraph",
        },
        paragraph: {
            group: "block",
            content: "inline*",
            toDOM() {
                return ["p", 0];
            },
            parseDOM: [{ tag: "p" }],
        },
        text: {
            group: "inline",
        },
        option_inline: {
            inline: true,
            group: "inline",
            atom: true,
            selectable: true,
            attrs: {
                options: { default: [""] },
                selectedIndex: { default: 0 },
            },
            toDOM(node) {
                const options = Array.isArray(node.attrs.options) ? node.attrs.options : [""];
                const idx = Number.isInteger(node.attrs.selectedIndex) ? node.attrs.selectedIndex : 0;
                const label = options[idx] ?? options[0] ?? "";
                return ["span", { class: "te-block te-block--option" }, ["span", { class: "te-block__body" }, label]];
            },
        },
        variable_inline: {
            inline: true,
            group: "inline",
            atom: true,
            selectable: true,
            attrs: {
                content: { default: "" },
            },
            toDOM(node) {
                const content = typeof node.attrs.content === "string" ? node.attrs.content : "";
                return [
                    "span",
                    { class: "te-block te-block--variable" },
                    ["span", { class: "te-block__body" }, content],
                ];
            },
        },
        hard_break: {
            inline: true,
            group: "inline",
            selectable: false,
            toDOM() {
                return ["br"];
            },
            parseDOM: [{ tag: "br" }],
        },
    },
    marks: {},
});

function buildDocFromRaw(raw: string): { doc: PMNode; isTemplate: boolean } {
    const parsed = parseTemplate(raw);
    const inlines: PMNode[] = [];

    for (const token of parsed.tokens) {
        if (token.type === "text") {
            if (token.content.length > 0) {
                // 将文本中的换行符转为 hard_break 节点
                const parts = token.content.split("\n");
                parts.forEach((part, i) => {
                    if (i > 0) inlines.push(schema.nodes.hard_break.create());
                    if (part.length > 0) inlines.push(schema.text(part));
                });
            }
            continue;
        }

        if (token.type === "option") {
            inlines.push(
                schema.nodes.option_inline.create({
                    options: token.options,
                    selectedIndex: token.selectedIndex,
                }),
            );
            continue;
        }

        if (token.type === "variable") {
            inlines.push(
                schema.nodes.variable_inline.create({
                    content: token.content,
                }),
            );
        }
    }

    const paragraph = schema.nodes.paragraph.create(null, inlines);
    return {
        doc: schema.nodes.doc.create(null, [paragraph]),
        isTemplate: parsed.isTemplate,
    };
}

function serializeDocToRaw(doc: PMNode, isTemplate: boolean): string {
    let inner = "";

    doc.forEach((block, _offset, blockIndex) => {
        if (blockIndex > 0) inner += "\n";
        block.forEach((node) => {
            if (node.isText) {
                inner += node.text ?? "";
                return;
            }

            if (node.type.name === "hard_break") {
                inner += "\n";
                return;
            }

            if (node.type.name === "option_inline") {
                const options = Array.isArray(node.attrs.options) ? node.attrs.options : [""];
                const selectedIndex = Number.isInteger(node.attrs.selectedIndex) ? node.attrs.selectedIndex : 0;
                const serializedOptions = options.map((opt: string, idx: number) =>
                    idx === selectedIndex ? `>${opt}` : opt,
                );
                inner += `[[${serializedOptions.join("|")}]]`;
                return;
            }

            if (node.type.name === "variable_inline") {
                const content = typeof node.attrs.content === "string" ? node.attrs.content : "";
                inner += `{{${content}}}`;
            }
        });
    });

    return isTemplate ? `<uos-ai-prompt-template>${inner}</uos-ai-prompt-template>` : inner;
}

function renderDocToPlainText(doc: PMNode): string {
    let rendered = "";

    doc.forEach((block, _offset, blockIndex) => {
        if (blockIndex > 0) rendered += "\n";
        block.forEach((node) => {
            if (node.isText) {
                rendered += node.text ?? "";
                return;
            }
            if (node.type.name === "hard_break") {
                rendered += "\n";
                return;
            }
            if (node.type.name === "option_inline") {
                const options = Array.isArray(node.attrs.options) ? node.attrs.options : [""];
                const idx = Number.isInteger(node.attrs.selectedIndex) ? node.attrs.selectedIndex : 0;
                rendered += options[idx] ?? options[0] ?? "";
                return;
            }
            if (node.type.name === "variable_inline") {
                rendered += typeof node.attrs.content === "string" ? node.attrs.content : "";
            }
        });
    });

    return rendered;
}

function renderSliceToPlainText(slice: Slice): string {
    const parts: string[] = [];

    const collectNode = (node: PMNode) => {
        if (node.isText) {
            parts.push(node.text ?? "");
            return;
        }

        if (node.type.name === "hard_break") {
            parts.push("\n");
            return;
        }

        if (node.type.name === "option_inline") {
            const options = Array.isArray(node.attrs.options) ? node.attrs.options : [""];
            const idx = Number.isInteger(node.attrs.selectedIndex) ? node.attrs.selectedIndex : 0;
            parts.push(options[idx] ?? options[0] ?? "");
            return;
        }

        if (node.type.name === "variable_inline") {
            parts.push(typeof node.attrs.content === "string" ? node.attrs.content : "");
            return;
        }

        node.content.forEach((child, _offset, childIndex) => {
            if (node.type.name === "paragraph" && childIndex === 0 && parts.length > 0) {
                parts.push("\n");
            }
            collectNode(child);
        });
    };

    slice.content.forEach((node) => collectNode(node));

    return parts.join("");
}

function isMismatchedTransactionError(error: unknown): boolean {
    return error instanceof RangeError && String(error.message ?? "").includes("mismatched transaction");
}

function createOptionNodeView(
    node: PMNode,
    view: EditorView,
    getPos: (() => number | undefined) | boolean,
    isDisabledRef: { value: boolean },
    onTabCycle: (from: HTMLElement, backward: boolean) => void,
    onEnter: () => void,
): NodeView {
    let currentNode = node;
    let dropdownEl: HTMLDivElement | null = null;
    let hoverIndex = -1;

    const dom = document.createElement("span");
    dom.className = "te-block te-block--option te-pm-block";

    const body = document.createElement("span");
    body.className = "te-block__body";

    // 触发器按钮，显示当前选中项
    const trigger = document.createElement("button");
    trigger.type = "button";
    trigger.className = "te-pm-option-trigger";

    const getOptions = (): string[] => (Array.isArray(currentNode.attrs.options) ? currentNode.attrs.options : [""]);
    const normalizeIndex = (index: unknown, options: string[]): number => {
        if (options.length === 0) return 0;
        const resolved = Number.isInteger(index) ? Number(index) : 0;
        return Math.min(Math.max(resolved, 0), options.length - 1);
    };
    const getSelectedIndex = (): number => normalizeIndex(currentNode.attrs.selectedIndex, getOptions());

    const render = () => {
        trigger.textContent = getOptions()[getSelectedIndex()] ?? "";
        trigger.disabled = isDisabledRef.value;
    };

    const resolveTargetNode = (pos: number): { pos: number; node: PMNode } | null => {
        const $pos = view.state.doc.resolve(pos);

        const after = $pos.nodeAfter;
        if (after && after.type === currentNode.type) {
            return { pos, node: after };
        }

        const before = $pos.nodeBefore;
        if (before && before.type === currentNode.type) {
            return { pos: pos - before.nodeSize, node: before };
        }

        return null;
    };

    const commitIndex = (index: number) => {
        if (isDisabledRef.value) return;
        if (typeof getPos !== "function") return;

        const nextIndex = normalizeIndex(index, getOptions());
        const pos = getPos();
        if (typeof pos !== "number") return;

        const resolved = resolveTargetNode(pos);
        const baseNode = resolved?.node ?? currentNode;
        const nodeOptions = Array.isArray(baseNode.attrs.options) ? baseNode.attrs.options : [""];
        const currentIndex = normalizeIndex(baseNode.attrs.selectedIndex, nodeOptions);
        if (nextIndex === currentIndex) return;

        const nextAttrs = {
            ...baseNode.attrs,
            selectedIndex: nextIndex,
        };

        if (resolved) {
            const tr = view.state.tr.setNodeMarkup(resolved.pos, baseNode.type, nextAttrs, baseNode.marks);
            try {
                view.dispatch(tr);
            } catch (error) {
                if (!isMismatchedTransactionError(error)) throw error;
            }
        }

        // 同步本地显示，避免 NodeView.update 在某些时序下延迟。
        currentNode = baseNode.type.create(nextAttrs, baseNode.content, baseNode.marks);
        render();
    };

    const updateHover = (index: number) => {
        hoverIndex = index;
        dropdownEl?.querySelectorAll(".te-block__dropdown-item").forEach((el, i) => {
            el.classList.toggle("is-hover", i === hoverIndex);
        });
    };

    const closeDropdown = () => {
        dropdownEl?.remove();
        dropdownEl = null;
        hoverIndex = -1;
    };

    const openDropdown = () => {
        if (isDisabledRef.value || dropdownEl) return;

        const options = getOptions();
        const selectedIndex = getSelectedIndex();
        const rect = trigger.getBoundingClientRect();

        dropdownEl = document.createElement("div");
        dropdownEl.className = "te-block__dropdown";
        dropdownEl.style.cssText = `position:fixed;top:${rect.bottom + 4}px;left:${rect.left}px;z-index:9999;`;

        options.forEach((label, index) => {
            const item = document.createElement("div");
            item.className = "te-block__dropdown-item" + (index === selectedIndex ? " is-selected" : "");
            item.textContent = label;
            // mousedown 用 preventDefault 阻止 button 失焦，让 click 也能触发
            item.addEventListener("mousedown", (e) => e.preventDefault());
            item.addEventListener("click", () => {
                commitIndex(index);
                closeDropdown();
                trigger.focus();
            });
            dropdownEl!.appendChild(item);
        });

        document.body.appendChild(dropdownEl);
        updateHover(selectedIndex);
    };

    trigger.addEventListener("focus", openDropdown);
    trigger.addEventListener("blur", (e) => {
        // 点击 dropdown item 时会 mousedown preventDefault，blur 的 relatedTarget 仍是 null
        // 因此延迟一 tick，让 click 事件先执行
        setTimeout(() => {
            if (!document.activeElement || !dom.contains(document.activeElement)) {
                closeDropdown();
            }
        }, 0);
    });
    trigger.addEventListener("mousedown", (event) => {
        event.stopPropagation();
        // 已聚焦时点击：切换下拉开关
        if (document.activeElement === trigger) {
            event.preventDefault();
            if (dropdownEl) {
                closeDropdown();
            } else {
                openDropdown();
            }
        }
    });
    trigger.addEventListener("keydown", (event) => {
        if (event.key === "Tab") {
            event.preventDefault();
            closeDropdown();
            onTabCycle(trigger, event.shiftKey);
            return;
        }
        if (event.key === "Escape") {
            closeDropdown();
            return;
        }
        if (event.key === "ArrowDown" || event.key === "ArrowUp") {
            event.preventDefault();
            if (!dropdownEl) openDropdown();
            const options = getOptions();
            const next = (hoverIndex + (event.key === "ArrowDown" ? 1 : -1) + options.length) % options.length;
            updateHover(next);
            return;
        }
        if (event.key === "Enter") {
            event.preventDefault();
            if (dropdownEl) {
                // 下拉展开时：提交选中项并关闭
                commitIndex(hoverIndex);
                closeDropdown();
            } else {
                // 下拉已关闭时：触发发送
                onEnter();
            }
            return;
        }
        if (event.key === " ") {
            event.preventDefault();
            if (dropdownEl) {
                commitIndex(hoverIndex);
                closeDropdown();
            } else {
                openDropdown();
            }
        }
    });

    body.appendChild(trigger);
    dom.appendChild(body);
    render();

    return {
        dom,
        update(updatedNode) {
            if (updatedNode.type !== currentNode.type) return false;
            currentNode = updatedNode;
            render();
            return true;
        },
        stopEvent(event) {
            return dom.contains(event.target as Node) || dropdownEl?.contains(event.target as Node) === true;
        },
        destroy() {
            closeDropdown();
        },
    };
}

function createVariableNodeView(
    node: PMNode,
    view: EditorView,
    getPos: (() => number | undefined) | boolean,
    isDisabledRef: { value: boolean },
    onTabCycle: (from: HTMLElement, backward: boolean) => void,
    onEnter: () => void,
): NodeView {
    let currentNode = node;

    const dom = document.createElement("span");
    dom.className = "te-block te-block--variable te-pm-block";

    const body = document.createElement("span");
    body.className = "te-block__body te-block__body--editing";

    // 隐藏的镜像 span，与 input 占同一个 grid 格，由它撑起宽度，无需 JS 计算
    const sizer = document.createElement("span");
    sizer.className = "te-pm-variable-sizer";
    sizer.setAttribute("aria-hidden", "true");

    const input = document.createElement("input");
    input.type = "text";
    input.className = "te-pm-variable-input";

    const PLACEHOLDER = "变量";

    const syncSizer = () => {
        // sizer 文字与 input 保持一致，决定容器宽度
        sizer.textContent = input.value || PLACEHOLDER;
    };

    const render = () => {
        const value = typeof currentNode.attrs.content === "string" ? currentNode.attrs.content : "";
        if (input.value !== value) {
            input.value = value;
        }
        input.disabled = isDisabledRef.value;
        input.placeholder = PLACEHOLDER;
        syncSizer();
    };

    const commit = (value: string) => {
        if (isDisabledRef.value) return;
        if (typeof getPos !== "function") return;
        if (value === currentNode.attrs.content) return;

        const pos = getPos();
        if (typeof pos !== "number") return;
        const tr = view.state.tr.setNodeMarkup(pos, undefined, {
            ...currentNode.attrs,
            content: value,
        });
        view.dispatch(tr);
    };

    input.addEventListener("input", () => {
        syncSizer();
        commit(input.value);
    });

    input.addEventListener("mousedown", (event) => {
        event.stopPropagation();
    });

    input.addEventListener("focus", () => {
        // 获得焦点时全选内容
        input.select();
    });

    input.addEventListener("keydown", (event) => {
        if (event.key === "Tab") {
            event.preventDefault();
            onTabCycle(input, event.shiftKey);
            return;
        }
        if (event.key === "Enter" && !event.shiftKey) {
            // 阻止冒泡，改为手动触发发送回调
            event.stopPropagation();
            onEnter();
            return;
        }
    });

    body.appendChild(sizer);
    body.appendChild(input);
    dom.appendChild(body);
    render();

    return {
        dom,
        update(updatedNode) {
            if (updatedNode.type !== currentNode.type) return false;
            currentNode = updatedNode;
            render();
            return true;
        },
        stopEvent(event) {
            return dom.contains(event.target as Node);
        },
    };
}

export default defineComponent({
    name: "TemplateEditor",

    props: {
        modelValue: {
            type: String,
            default: "",
        },
        placeholder: {
            type: String,
            default: "请输入内容...",
        },
        disabled: {
            type: Boolean,
            default: false,
        },
        maxlength: {
            type: Number,
            default: undefined,
        },
    },

    emits: {
        updateValue: (value: string) => typeof value === "string",
        input: (value: string) => typeof value === "string",
        focus: null,
        blur: null,
        enter: null,
        send: null,
    },

    setup(props, { emit }) {
        const containerRef = ref<HTMLDivElement | null>(null);
        const editorRootRef = ref<HTMLDivElement | null>(null);
        // EditorView 是复杂类实例，不能被 Vue 深层代理，否则事务对象会出现代理/原始对象混用。
        const editorViewRef = shallowRef<EditorView | null>(null);

        const hasFocus = ref(false);
        const isEmpty = ref(true);
        const isTemplateMode = ref(false);
        const isDisabledRef = ref(props.disabled);
        const lastEmitted = ref(props.modelValue);

        function syncIsEmpty(doc: PMNode) {
            let hasNonTextNode = false;
            let text = "";

            doc.descendants((node) => {
                if (node.type.name === "option_inline" || node.type.name === "variable_inline") {
                    hasNonTextNode = true;
                }
                if (node.isText) {
                    text += node.text ?? "";
                }
            });

            isEmpty.value = !hasNonTextNode && text.length === 0;
        }

        function emitIfChanged(doc: PMNode) {
            const raw = serializeDocToRaw(doc, isTemplateMode.value);
            if (raw === lastEmitted.value) return;
            lastEmitted.value = raw;
            emit("updateValue", raw);
            emit("input", raw);
        }

        function createStateFromRaw(raw: string): EditorState {
            const parsed = buildDocFromRaw(raw);
            isTemplateMode.value = parsed.isTemplate;

            return EditorState.create({
                schema,
                doc: parsed.doc,
                plugins: [
                    history(),
                    keymap({
                        "Mod-z": undo,
                        "Mod-Shift-z": redo,
                        "Mod-y": redo,
                    }),
                    // Shift/Alt/Ctrl + Enter 插入换行符（hard_break）
                    keymap({
                        "Shift-Enter": (state, dispatch) => {
                            if (dispatch)
                                dispatch(
                                    state.tr.replaceSelectionWith(schema.nodes.hard_break.create()).scrollIntoView(),
                                );
                            return true;
                        },
                        "Alt-Enter": (state, dispatch) => {
                            if (dispatch)
                                dispatch(
                                    state.tr.replaceSelectionWith(schema.nodes.hard_break.create()).scrollIntoView(),
                                );
                            return true;
                        },
                        "Ctrl-Enter": (state, dispatch) => {
                            if (dispatch)
                                dispatch(
                                    state.tr.replaceSelectionWith(schema.nodes.hard_break.create()).scrollIntoView(),
                                );
                            return true;
                        },
                    }),
                    keymap(baseKeymap),
                    new Plugin({
                        filterTransaction: (tr, state) => {
                            if (!props.maxlength || !tr.docChanged) return true;
                            const newState = state.apply(tr);
                            const textLen = renderDocToPlainText(newState.doc).length;
                            return textLen <= props.maxlength;
                        },
                    }),
                ],
            });
        }

        function focus() {
            const view = editorViewRef.value;
            if (!view) return;
            view.focus();
            const tr = view.state.tr.setSelection(Selection.atEnd(view.state.doc));
            try {
                view.dispatch(tr);
            } catch (error) {
                if (!isMismatchedTransactionError(error)) throw error;
            }
        }

        function insertTextAtCursor(text: string) {
            const view = editorViewRef.value;
            if (!view || !text) return;
            const { from, to } = view.state.selection;
            view.dispatch(view.state.tr.insertText(text, from, to));
        }

        function mountEditor() {
            if (!editorRootRef.value) return;

            const initState = createStateFromRaw(props.modelValue);
            syncIsEmpty(initState.doc);

            const view = new EditorView(editorRootRef.value, {
                state: initState,
                editable: () => !isDisabledRef.value,
                dispatchTransaction: (tr) => {
                    try {
                        const nextState = view.state.apply(tr);
                        view.updateState(nextState);
                        syncIsEmpty(nextState.doc);
                        if (tr.docChanged) {
                            emitIfChanged(nextState.doc);
                        }
                    } catch (error) {
                        // 外部 updateState 与本地输入并发时，ProseMirror 可能抛出 stale transaction。
                        // 忽略该事务，等待下一次用户输入/外部同步。
                        if (!isMismatchedTransactionError(error)) {
                            throw error;
                        }
                    }
                },
                nodeViews: {
                    option_inline: (node, nodeView, getPos) =>
                        createOptionNodeView(
                            node,
                            nodeView,
                            getPos,
                            isDisabledRef,
                            (from, backward) => cycleTabFocus(from, backward),
                            () => emit("enter"),
                        ),
                    variable_inline: (node, nodeView, getPos) =>
                        createVariableNodeView(
                            node,
                            nodeView,
                            getPos,
                            isDisabledRef,
                            (from, backward) => cycleTabFocus(from, backward),
                            () => emit("enter"),
                        ),
                },
                handleDOMEvents: {
                    keydown: (_view, event) => {
                        if (event.key === "Tab") {
                            event.preventDefault();
                            cycleTabFocus(null, event.shiftKey);
                            return true;
                        }
                        // 仅无修饰键的 Enter 触发发送；Shift/Alt/Ctrl+Enter 由 keymap 插件处理换行
                        if (event.key === "Enter" && !event.shiftKey && !event.altKey && !event.ctrlKey) {
                            event.preventDefault();
                            emit("enter");
                            return true;
                        }
                        return false;
                    },
                },
                clipboardTextSerializer: (slice) => renderSliceToPlainText(slice),
                handlePaste: (view, event) => {
                    // 多行文本粘贴：将 \n 转换为 hard_break 节点，避免多段落被 schema 截断
                    const text = event.clipboardData?.getData("text/plain") ?? "";
                    if (!text) return false;

                    const parts = text.split("\n");
                    const inlines: PMNode[] = [];
                    parts.forEach((part, i) => {
                        if (i > 0) inlines.push(schema.nodes.hard_break.create());
                        if (part.length > 0) inlines.push(schema.text(part));
                    });

                    if (inlines.length === 0) return true;

                    const slice = new Slice(Fragment.from(inlines), 0, 0);
                    view.dispatch(view.state.tr.replaceSelection(slice));
                    return true;
                },
            });

            editorViewRef.value = markRaw(view);
        }

        function destroyEditor() {
            editorViewRef.value?.destroy();
            editorViewRef.value = null;
        }

        function handleContainerMouseDown(event: MouseEvent) {
            if (props.disabled) return;
            const target = event.target as HTMLElement;
            if (target.closest(".ProseMirror")) return;
            event.preventDefault();
            nextTick(() => focus());
        }

        function getTabbableBlocks(): HTMLElement[] {
            if (!containerRef.value) return [];
            return Array.from(
                containerRef.value.querySelectorAll(".te-pm-option-trigger, .te-pm-variable-input"),
            ) as HTMLElement[];
        }

        function focusTabbableByIndex(index: number) {
            const blocks = getTabbableBlocks();
            if (blocks.length === 0) {
                focus();
                return;
            }

            const normalized = ((index % blocks.length) + blocks.length) % blocks.length;
            const target = blocks[normalized];
            target?.focus();
            if (target instanceof HTMLInputElement) {
                target.select();
            }
            // button trigger 聚焦时，focus 事件会自动调用 openDropdown
        }

        function cycleTabFocus(from: HTMLElement | null, backward: boolean) {
            const blocks = getTabbableBlocks();
            if (blocks.length === 0) {
                focus();
                return;
            }

            if (!from) {
                focusTabbableByIndex(backward ? blocks.length - 1 : 0);
                return;
            }

            const currentIndex = blocks.indexOf(from);
            if (currentIndex < 0) {
                focusTabbableByIndex(backward ? blocks.length - 1 : 0);
                return;
            }

            focusTabbableByIndex(currentIndex + (backward ? -1 : 1));
        }

        function handleContainerFocusIn() {
            if (hasFocus.value) return;
            hasFocus.value = true;
            emit("focus");
        }

        function handleContainerFocusOut(event: FocusEvent) {
            if (containerRef.value?.contains(event.relatedTarget as Node)) return;
            if (!hasFocus.value) return;
            hasFocus.value = false;
            emit("blur");
        }

        watch(
            () => props.modelValue,
            (newVal) => {
                const view = editorViewRef.value;
                if (!view) return;
                if (newVal === lastEmitted.value) return;

                lastEmitted.value = newVal;
                const nextState = createStateFromRaw(newVal);
                view.updateState(nextState);
                syncIsEmpty(nextState.doc);
            },
        );

        watch(
            () => props.disabled,
            (val) => {
                isDisabledRef.value = val;
                const view = editorViewRef.value;
                if (!view) return;
                view.setProps({
                    editable: () => !isDisabledRef.value,
                });
                // 立即刷新节点视图里的 input/select 禁用态
                containerRef.value?.querySelectorAll(".te-pm-option-select, .te-pm-variable-input").forEach((el) => {
                    if (el instanceof HTMLInputElement || el instanceof HTMLSelectElement) {
                        el.disabled = val;
                    }
                });
            },
        );

        onMounted(mountEditor);
        onBeforeUnmount(destroyEditor);

        return {
            containerRef,
            editorRootRef,
            hasFocus,
            isEmpty,
            isTemplateMode,
            handleContainerMouseDown,
            handleContainerFocusIn,
            handleContainerFocusOut,
            focus,
            insertTextAtCursor,
        };
    },

    render() {
        return (
            <div
                ref="containerRef"
                class={[
                    "template-editor",
                    this.hasFocus && "template-editor--focused",
                    this.$props.disabled && "template-editor--disabled",
                    this.isTemplateMode && "template-editor--template",
                ]}
                onFocusin={this.handleContainerFocusIn}
                onFocusout={this.handleContainerFocusOut}
                onMousedown={this.handleContainerMouseDown}
            >
                {this.isEmpty && <span class="template-editor__placeholder">{this.$props.placeholder}</span>}
                <ScrollBar class="template-editor__scroll">
                    <div class="template-editor__content-wrapper">
                        <div ref="editorRootRef" class="template-editor__pm-root" />
                    </div>
                </ScrollBar>
            </div>
        );
    },
});
