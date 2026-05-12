/**
 * Milkdown 数学公式插件
 *
 * 基于 remark-math + KaTeX 实现行内公式（$...$）和块级公式（$$...$$）的渲染。
 * 不依赖 CodeMirror，可直接用于 Editor.make().use()。
 *
 * 同时提供 normalizeMathDelimiters() 用于将 LLM 常见的 \(...\) / \[...\] 定界符
 * 转换为 $...$ / $$...$$ 定界符，以便 remark-math 正确解析。
 */
import katex from "katex";
import remarkMath from "remark-math";
import { $remark, $nodeSchema, $inputRule } from "@milkdown/kit/utils";
import { nodeRule } from "@milkdown/kit/prose";
import { InputRule } from "@milkdown/kit/prose/inputrules";
import type { MilkdownPlugin } from "@milkdown/kit/ctx";

/* ==================== LaTeX 定界符规范化 ==================== */

/**
 * 将 \(...\) 和 \[...\] 定界符转换为 $...$ 和 $$...$$。
 *
 * LLM 输出经常使用 LaTeX 风格定界符，而 remark-math 只能解析 dollar 风格。
 * 标准 Markdown 解析器会把 \( 当作转义的 (，导致公式显示为纯文本。
 */
export function normalizeMathDelimiters(markdown: string): string {
    // 块级公式：\[...\] → $$...$$（可跨行）
    let result = markdown.replace(/\\\[([\s\S]*?)\\\]/g, (_match, content: string) => {
        return `$$${content}$$`;
    });

    // 行内公式：\(...\) → $...$（不跨行）
    result = result.replace(/\\\((.+?)\\\)/g, (_match, content: string) => {
        return `$${content}$`;
    });

    return result;
}

/* ==================== remark-math 解析插件 ==================== */

// 让 Milkdown 的 markdown 解析器识别 $...$ 和 $$...$$ 语法
const remarkMathPlugin = $remark("remarkMath", () => remarkMath);

/* ==================== 行内公式 $...$ ==================== */

const MATH_INLINE_ID = "math_inline";

const mathInlineSchema = $nodeSchema(MATH_INLINE_ID, () => ({
    group: "inline",
    inline: true,
    atom: true,
    attrs: {
        value: { default: "" },
    },
    parseDOM: [
        {
            tag: `span[data-type="${MATH_INLINE_ID}"]`,
            getAttrs: (dom: HTMLElement) => ({
                value: dom.dataset.value ?? "",
            }),
        },
    ],
    toDOM: (node) => {
        const code = node.attrs.value as string;
        const dom = document.createElement("span");
        dom.dataset.type = MATH_INLINE_ID;
        dom.dataset.value = code;
        dom.classList.add("math-inline");
        try {
            katex.render(code, dom, { throwOnError: false });
        } catch {
            dom.textContent = code;
        }
        return dom;
    },
    parseMarkdown: {
        match: (node) => node.type === "inlineMath",
        runner: (state, node, type) => {
            state.addNode(type, { value: node.value as string });
        },
    },
    toMarkdown: {
        match: (node) => node.type.name === MATH_INLINE_ID,
        runner: (state, node) => {
            state.addNode("inlineMath", undefined, node.attrs.value as string);
        },
    },
}));

// 输入规则：键入 $内容$ 后自动转换为 inline math 节点
// 使用 negative lookbehind (?<!\$) 和 negative lookahead (?!\$) 确保不误匹配 $$...$$ 块级公式
const mathInlineInputRule = $inputRule((ctx) =>
    nodeRule(/(?<!\$)\$([^$\n]+)\$(?!\$)$/, mathInlineSchema.type(ctx), {
        getAttr: (match: RegExpMatchArray) => ({
            value: match[1] ?? "",
        }),
    }),
);

/* ==================== 块级公式 $$...$$ ==================== */

const MATH_BLOCK_ID = "math_block";

const mathBlockSchema = $nodeSchema(MATH_BLOCK_ID, () => ({
    group: "block",
    // atom: true 表示节点整体不可编辑，值通过 attrs 存储，与隐藏源码的设计匹配
    atom: true,
    attrs: {
        value: { default: "" },
    },
    parseDOM: [
        {
            tag: `div[data-type="${MATH_BLOCK_ID}"]`,
            getAttrs: (dom: HTMLElement) => ({
                value: dom.dataset.value ?? "",
            }),
        },
    ],
    toDOM: (node) => {
        const code = (node.attrs.value || "") as string;
        const wrapper = document.createElement("div");
        wrapper.dataset.type = MATH_BLOCK_ID;
        wrapper.dataset.value = code;
        wrapper.classList.add("math-block");

        if (code.trim()) {
            // 渲染 KaTeX
            const preview = document.createElement("div");
            preview.classList.add("math-block__preview");
            try {
                katex.render(code, preview, {
                    throwOnError: false,
                    displayMode: true,
                });
            } catch {
                preview.textContent = code;
            }
            wrapper.appendChild(preview);
        }

        // atom 节点直接返回 dom，无 contentDOM
        return wrapper;
    },
    parseMarkdown: {
        match: (node) => node.type === "math",
        runner: (state, node, type) => {
            state.addNode(type, { value: (node.value as string) || "" });
        },
    },
    toMarkdown: {
        match: (node) => node.type.name === MATH_BLOCK_ID,
        runner: (state, node) => {
            state.addNode("math", undefined, (node.attrs.value as string) || "");
        },
    },
}));

// 输入规则：在段落中输入 $$公式$$ 后自动创建块级公式节点
// 触发时机：键入最后一个 $ 时，当前段落文本恰好为 $$...$$ 整体
// 匹配：^\$\$(内容，不含 $ 且不跨行)\$\$$
const mathBlockInputRule = $inputRule((ctx) =>
    new InputRule(/^\$\$((?:[^$\n]|\$(?!\$))+)\$\$$/, (state, match, start, end) => {
        const value = (match[1] ?? "").trim();
        if (!value) return null;

        const type = mathBlockSchema.type(ctx);
        const $from = state.doc.resolve(start);
        // 替换掉整个所在段落节点（不只是匹配的文本范围）
        const blockFrom = $from.before($from.depth);
        const blockTo = $from.after($from.depth);
        return state.tr.replaceWith(blockFrom, blockTo, type.create({ value }));
    }),
);

/* ==================== 导出：一次性 .use() 的插件列表 ==================== */

export const mathPlugins: MilkdownPlugin[] = [
    ...remarkMathPlugin,
    ...mathInlineSchema,
    ...mathBlockSchema,
    mathInlineInputRule,
    mathBlockInputRule,
];
