/**
 * Milkdown 脚注插件
 *
 * GFM 预设已包含脚注 schema 与 remark-gfm 解析能力，
 * 这里补充输入规则，让用户在编辑过程中输入 `[^id]` / `[^id]:` 时即时渲染为脚注节点。
 */
import type { MilkdownPlugin } from "@milkdown/kit/ctx";
import { footnoteDefinitionSchema, footnoteReferenceSchema } from "@milkdown/kit/preset/gfm";
import { InputRule } from "@milkdown/kit/prose/inputrules";
import { Selection } from "@milkdown/kit/prose/state";
import { $inputRule, $remark } from "@milkdown/kit/utils";

type MdastNode = {
    type: string;
    value?: string;
    children?: MdastNode[];
    [key: string]: unknown;
};

/**
 * 宽松脚注解析：
 * - remark-gfm 只有在存在 `[^id]: ...` 定义时才会把 `[^id]` 识别为 footnoteReference
 * - 聊天内容常出现“只有引用、无定义”的情况，这里将其回填为 footnoteReference 节点以便渲染
 */
const looseFootnoteReferenceRemark = $remark("looseFootnoteReferenceRemark", () => () => (tree: MdastNode) => {
    const skipParentTypes = new Set(["code", "inlineCode", "footnoteDefinition", "html"]);
    const footnoteRefPattern = /(?<!\\)\[\^([^\]\s]+)\](?!:)/g;

    const walk = (node: MdastNode) => {
        if (!Array.isArray(node.children) || skipParentTypes.has(node.type)) {
            return;
        }

        const nextChildren: MdastNode[] = [];
        for (const child of node.children) {
            if (child.type !== "text" || typeof child.value !== "string") {
                walk(child);
                nextChildren.push(child);
                continue;
            }

            const text = child.value;
            let lastIndex = 0;
            let matched = false;
            footnoteRefPattern.lastIndex = 0;

            for (let match = footnoteRefPattern.exec(text); match; match = footnoteRefPattern.exec(text)) {
                matched = true;
                const full = match[0] ?? "";
                const label = (match[1] ?? "").trim();
                const start = match.index;

                if (start > lastIndex) {
                    nextChildren.push({ type: "text", value: text.slice(lastIndex, start) });
                }

                if (label) {
                    nextChildren.push({
                        type: "footnoteReference",
                        identifier: label,
                        label,
                    });
                } else {
                    nextChildren.push({ type: "text", value: full });
                }
                lastIndex = start + full.length;
            }

            if (!matched) {
                nextChildren.push(child);
                continue;
            }

            if (lastIndex < text.length) {
                nextChildren.push({ type: "text", value: text.slice(lastIndex) });
            }
        }

        node.children = nextChildren;
        nextChildren.forEach(walk);
    };

    walk(tree);
});

// 行内脚注引用：`[^note]`
const footnoteReferenceInputRule = $inputRule((ctx) =>
    new InputRule(/\[\^([^\]\s]+)\]$/, (state, match, start, end) => {
        const label = (match[1] ?? "").trim();
        if (!label) return null;

        // 避免与定义语法 `[^id]:` 冲突：行首的 `[^id]` 先不自动转换，
        // 让用户可以继续输入 `:` 触发 definition 规则。
        const $start = state.doc.resolve(start);
        if (start === $start.start($start.depth)) {
            return null;
        }

        const type = footnoteReferenceSchema.type(ctx);
        return state.tr.replaceWith(start, end, type.create({ label }));
    }),
);

// 脚注定义：`[^note]: `
// 在单独段落输入该前缀时，将当前段落转换为 footnote_definition，
// 后续可在定义段落内继续编辑正文。
const footnoteDefinitionInputRule = $inputRule((ctx) =>
    new InputRule(/^\[\^([^\]\s]+)\]:\s$/, (state, match, start) => {
        const label = (match[1] ?? "").trim();
        if (!label) return null;

        const footnoteDefinitionType = footnoteDefinitionSchema.type(ctx);
        const paragraphType = state.schema.nodes.paragraph;
        if (!paragraphType) return null;

        const paragraph = paragraphType.create();
        const footnoteDefinition = footnoteDefinitionType.create({ label }, [paragraph]);

        const $from = state.doc.resolve(start);
        const blockFrom = $from.before($from.depth);
        const blockTo = $from.after($from.depth);

        const tr = state.tr.replaceWith(blockFrom, blockTo, footnoteDefinition);
        const cursorPos = Math.min(blockFrom + 2, tr.doc.content.size);
        return tr.setSelection(Selection.near(tr.doc.resolve(cursorPos)));
    }),
);

export const footnotePlugins: MilkdownPlugin[] = [
    ...looseFootnoteReferenceRemark,
    footnoteReferenceInputRule,
    footnoteDefinitionInputRule,
];
