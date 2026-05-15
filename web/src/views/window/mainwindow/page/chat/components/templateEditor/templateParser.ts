/**
 * 模板解析器
 *
 * 模板格式：
 *   <uos-ai-prompt-template>请帮我生成[[本周|本月]]的{{技术周报}}</uos-ai-prompt-template>
 *
 * [[选项1|选项2]] → OptionToken
 * {{变量名}}      → VariableToken
 * 其余纯文本      → TextToken
 */

// ─────────────────────── Token 类型定义 ───────────────────────

interface TextToken {
    type: "text";
    id: string;
    content: string;
}

interface OptionToken {
    type: "option";
    id: string;
    options: string[];
    selectedIndex: number;
}

interface VariableToken {
    type: "variable";
    id: string;
    content: string;
}

type EditorToken = TextToken | OptionToken | VariableToken;

interface EditorModel {
    /** 是否为模板格式（含 <uos-ai-prompt-template> 标签） */
    isTemplate: boolean;
    tokens: EditorToken[];
}

// ─────────────────────── 内部工具 ───────────────────────

const TEMPLATE_TAG_RE =
    /^<uos-ai-prompt-template>([\s\S]*)<\/uos-ai-prompt-template>$/;

let _idCounter = 0;
function genId(prefix: string): string {
    return `${prefix}-${Date.now()}-${++_idCounter}`;
}

/**
 * 解析模板内部内容为 Token 数组
 * 例如：请帮我生成[[本周|本月]]的{{技术周报}}
 */
function parseInner(inner: string): EditorToken[] {
    const tokens: EditorToken[] = [];
    // 匹配 [[...]] 和 {{...}}
    const tokenRe = /(\[\[([^\]]*)\]\]|\{\{([^}]*)\}\})/g;
    let lastIndex = 0;
    let match: RegExpExecArray | null;

    while ((match = tokenRe.exec(inner)) !== null) {
        // 前置纯文本
        if (match.index > lastIndex) {
            tokens.push({
                type: "text",
                id: genId("txt"),
                content: inner.slice(lastIndex, match.index),
            });
        }

        if (match[2] !== undefined) {
            // [[选项1|选项2|...]]，带 > 前缀的项为当前选中项
            let selectedIndex = 0;
            const options = match[2]
                .split("|")
                .map((s) => s.trim())
                .filter(Boolean)
                .map((s, i) => {
                    if (s.startsWith(">")) {
                        selectedIndex = i;
                        return s.slice(1);
                    }
                    return s;
                });
            tokens.push({
                type: "option",
                id: genId("opt"),
                options: options.length > 0 ? options : [""],
                selectedIndex,
            });
        } else if (match[3] !== undefined) {
            // {{变量名}}
            tokens.push({
                type: "variable",
                id: genId("var"),
                content: match[3].trim(),
            });
        }

        lastIndex = match.index + match[0].length;
    }

    // 尾部纯文本
    if (lastIndex < inner.length) {
        tokens.push({
            type: "text",
            id: genId("txt"),
            content: inner.slice(lastIndex),
        });
    }

    // 保证始终有至少一个 TextToken（便于光标定位）
    if (tokens.length === 0) {
        tokens.push({ type: "text", id: genId("txt"), content: "" });
    }

    return tokens;
}

// ─────────────────────── 公开 API ───────────────────────

/**
 * 将原始字符串解析为 EditorModel。
 * - 如果是模板字符串，isTemplate = true，tokens 为解析后的 Token 列表
 * - 否则 isTemplate = false，tokens 为单个 TextToken（内容为原始字符串）
 */
export function parseTemplate(raw: string): EditorModel {
    const trimmed = raw.trim();
    const templateMatch = TEMPLATE_TAG_RE.exec(trimmed);

    if (templateMatch) {
        return {
            isTemplate: true,
            tokens: parseInner(templateMatch[1] ?? ""),
        };
    }

    // 纯文本模式
    return {
        isTemplate: false,
        tokens: [{ type: "text", id: genId("txt"), content: raw }],
    };
}

/**
 * 获取模板渲染的纯文本（用于发送消息时提取实际内容）。
 * - OptionToken → 当前选中选项文本
 * - VariableToken → 变量内容
 * - TextToken → 原文本
 */
function renderModelToPlainText(model: EditorModel): string {
    return model.tokens
        .map((token) => {
            if (token.type === "text") return token.content;
            if (token.type === "option")
                return token.options[token.selectedIndex] ?? "";
            if (token.type === "variable") return token.content;
            return "";
        })
        .join("");
}

/**
 * 将原始字符串（含或不含模板标签）渲染为最终纯文本。
 * - 模板字符串：解析后取各 Token 的当前值拼接
 * - 普通字符串：直接返回原始内容
 */
export function getRenderedText(raw: string): string {
    return renderModelToPlainText(parseTemplate(raw));
}
