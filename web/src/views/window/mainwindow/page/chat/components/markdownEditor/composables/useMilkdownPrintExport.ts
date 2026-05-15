/**
 * 从 Milkdown 编辑器导出可打印 HTML。
 *
 * 核心思路：直接克隆编辑器真实渲染的 DOM（包含 NodeView 包装器、class 层级），
 * 而非使用 getHTML()（它只返回纯语义标签，丢失 .milkdown-table-block 等包装器），
 * 这样收集到的 CSS 规则（选择器如 .milkdown .ProseMirror blockquote）能够正确匹配。
 *
 * 同时把 .milkdown 上的 CSS 自定义属性（--crepe-*）解析为实际值注入，
 * 确保独立 HTML 页面中颜色、字体等变量可用。
 */

/* ============================
   CSS 收集
   ============================ */

function isEditorRelatedCss(cssText: string): boolean {
    const tokens = [".milkdown", ".ProseMirror", ".editor", "pre", "code", "table", "blockquote", "img"];
    return tokens.some((token) => cssText.includes(token));
}

/** 从当前页面所有样式表中收集与编辑器相关的 CSS 规则 */
function collectEditorCss(): string {
    const chunks: string[] = [];

    for (const sheet of Array.from(document.styleSheets)) {
        try {
            for (const rule of Array.from(sheet.cssRules || [])) {
                const cssText = rule.cssText || "";
                if (cssText && isEditorRelatedCss(cssText)) {
                    chunks.push(cssText);
                }
            }
        } catch (_error) {
            // 跨域样式表可能无法读取，直接跳过
            continue;
        }
    }

    return chunks.join("\n");
}

/** 从 .milkdown 元素的 computedStyle 中提取所有 --crepe-* / --milkdown-* CSS 变量并解析为实际值 */
function collectCssVariables(): string {
    const milkdownEl = document.querySelector(".milkdown") as HTMLElement | null;
    if (!milkdownEl) return "";

    const computed = getComputedStyle(milkdownEl);
    const vars: string[] = [];

    // 遍历所有样式表查找在 .milkdown 上声明的自定义属性名
    const varNames = new Set<string>();
    for (const sheet of Array.from(document.styleSheets)) {
        try {
            for (const rule of Array.from(sheet.cssRules || [])) {
                if (rule instanceof CSSStyleRule && rule.selectorText?.includes(".milkdown")) {
                    for (let i = 0; i < rule.style.length; i++) {
                        const prop = rule.style.item(i);
                        if (prop.startsWith("--")) {
                            varNames.add(prop);
                        }
                    }
                }
            }
        } catch (_e) {
            continue;
        }
    }

    // 用 getComputedStyle 获取最终解析值
    for (const name of varNames) {
        const value = computed.getPropertyValue(name).trim();
        if (value) {
            vars.push(`    ${name}: ${value};`);
        }
    }

    if (vars.length === 0) return "";
    return `.milkdown {\n${vars.join("\n")}\n}`;
}

/* ============================
   DOM 克隆 & 清理
   ============================ */

// 需要从克隆 DOM 中移除的编辑态 class
const EDITING_CLASSES = [
    "ProseMirror-focused",
    "ProseMirror-selectednode",
    "ProseMirror-hideselection",
    "selectedCell",
];

// 需要移除的编辑态元素选择器
const EDITING_SELECTORS = [
    // milkdown 表格拖拽手柄
    ".cell-handle",
    ".line-handle",
    ".drag-preview",
    ".handle",
    // ProseMirror gapcursor
    ".ProseMirror-gapcursor",
    // 占位符
    ".placeholder",
    "[data-placeholder]",
];

/** 克隆编辑器 DOM 并清理编辑态 artifact */
function cloneAndCleanEditorDom(): HTMLElement | null {
    const editorEl = document.querySelector(".milkdown .ProseMirror") as HTMLElement | null;
    if (!editorEl) return null;

    const clone = editorEl.cloneNode(true) as HTMLElement;

    // 移除编辑态元素
    for (const selector of EDITING_SELECTORS) {
        clone.querySelectorAll(selector).forEach((el) => el.remove());
    }

    // 清理编辑态 class 和属性
    const allElements = clone.querySelectorAll("*");
    for (const el of Array.from(allElements)) {
        const htmlEl = el as HTMLElement;

        // 移除编辑态 class
        for (const cls of EDITING_CLASSES) {
            htmlEl.classList.remove(cls);
        }

        // 移除 contenteditable
        htmlEl.removeAttribute("contenteditable");

        // 移除 ProseMirror 数据属性
        for (const attr of Array.from(htmlEl.attributes)) {
            if (attr.name.startsWith("data-node-type") ||
                attr.name === "data-dragging" ||
                attr.name === "draggable") {
                htmlEl.removeAttribute(attr.name);
            }
        }
    }

    // 将编辑器中 table 的真实渲染宽度和外边框内联到克隆元素上
    const liveTables = editorEl.querySelectorAll("table");
    const cloneTables = clone.querySelectorAll("table");
    for (let i = 0; i < cloneTables.length; i++) {
        const liveTable = liveTables[i];
        const cloneTable = cloneTables[i] as HTMLElement;
        if (!liveTable) continue;

        const computed = getComputedStyle(liveTable);
        // 保留编辑器中的真实宽度（非全宽）
        cloneTable.style.width = computed.width;
        cloneTable.style.borderCollapse = "collapse";

        // 从 th/td 的 border 色取外边线颜色
        const firstCell = liveTable.querySelector("th, td");
        if (firstCell) {
            const cellBorder = getComputedStyle(firstCell).borderColor || "#d9d9d9";
            cloneTable.style.border = `1px solid ${cellBorder}`;
        }
    }

    // 清理根元素自身
    clone.removeAttribute("contenteditable");
    clone.removeAttribute("role");
    clone.removeAttribute("spellcheck");
    clone.removeAttribute("tabindex");
    clone.removeAttribute("translate");
    for (const cls of EDITING_CLASSES) {
        clone.classList.remove(cls);
    }

    return clone;
}

/* ============================
   兜底 CSS
   ============================ */

function buildFallbackCss(): string {
    return `
html, body {
    margin: 0;
    padding: 0;
    background: #fff;
}

/* 打印模式下覆盖编辑器的大 padding */
.milkdown .ProseMirror {
    padding: 24px 40px;
}

.milkdown .ProseMirror img {
    max-width: 100%;
    height: auto;
}

/* 代码块换行以防溢出 */
.milkdown .ProseMirror pre,
.milkdown .ProseMirror code {
    white-space: pre-wrap;
    word-break: break-word;
}

/* 表格基础保障（宽度和外边框由内联样式控制，此处仅做 collapse 兜底） */
.milkdown .ProseMirror table {
    border-collapse: collapse;
}

@media print {
    html, body {
        -webkit-print-color-adjust: exact;
        print-color-adjust: exact;
    }

    .milkdown .ProseMirror {
        padding: 0 12px;
    }
}
`;
}

/* ============================
   工具函数
   ============================ */

function escapeHtml(input: string): string {
    return input
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#39;");
}

/* ============================
   导出
   ============================ */

export function useMilkdownPrintExport() {
    const exportPrintableHtml = (_editor: any, title = "Document"): string => {
        // 直接克隆真实 DOM，而非 getHTML()
        const cleanedDom = cloneAndCleanEditorDom();
        if (!cleanedDom || !cleanedDom.innerHTML.trim()) {
            return "";
        }

        const editorCss = collectEditorCss();
        const cssVars = collectCssVariables();
        const safeTitle = escapeHtml(title || "Document");

        return `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>${safeTitle}</title>
    <style>
${cssVars}
${editorCss}
${buildFallbackCss()}
    </style>
</head>
<body>
    <div class="milkdown">${cleanedDom.outerHTML}</div>
</body>
</html>`;
    };

    return {
        exportPrintableHtml,
    };
}
