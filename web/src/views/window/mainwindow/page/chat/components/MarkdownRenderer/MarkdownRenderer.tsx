import { defineComponent, computed, onMounted, ref, watch, nextTick, createApp, h } from "vue";
import { marked } from "marked";
import hljs from "highlight.js";
import "highlight.js/styles/github-dark.css";
import { useBackendStore } from "@/stores";
import ScrollBar from "@/components/ScrollBar";
import Tooltip from "@/components/Tooltip";

interface Props {
    content: string;
    isUser?: boolean;
    showLoading?: boolean;
}

// Module-level loading states, shared across all component instances
let mathJaxPromise: Promise<void> | null = null;
let mermaidPromise: Promise<any> | null = null;

const isDarkMode = (): boolean => {
    return document.documentElement.classList.contains("dark") || window.matchMedia("(prefers-color-scheme: dark)").matches;
};

const loadMermaid = async (): Promise<any> => {
    if ((window as any).mermaid) {
        return (window as any).mermaid;
    }

    if (mermaidPromise) {
        return mermaidPromise;
    }

    const existingScript = document.querySelector('script[src*="mermaid"]');
    if (existingScript) {
        mermaidPromise = new Promise<any>((resolve, reject) => {
            existingScript.addEventListener("load", () => {
                const m = (window as any).mermaid;
                if (m) {
                    m.initialize({ startOnLoad: false, theme: isDarkMode() ? "dark" : "default" });
                }
                resolve(m);
            });
            existingScript.addEventListener("error", reject);
        });
        return mermaidPromise;
    }

    mermaidPromise = new Promise<any>((resolve, reject) => {
        const script = document.createElement("script");
        script.src = "https://cdn.jsdelivr.net/npm/mermaid@8.14.0/dist/mermaid.min.js";
        script.async = true;
        script.onload = () => {
            const m = (window as any).mermaid;
            if (m) {
                m.initialize({ startOnLoad: false, theme: isDarkMode() ? "dark" : "default" });
            }
            resolve(m);
        };
        script.onerror = reject;
        document.head.appendChild(script);
    });

    return mermaidPromise;
};

let mermaidRenderQueue: Array<{ code: string; container: HTMLElement }> = [];
let mermaidProcessing = false;

const processMermaidQueue = async () => {
    if (mermaidProcessing) return;
    mermaidProcessing = true;

    while (mermaidRenderQueue.length > 0) {
        const item = mermaidRenderQueue.shift()!;
        try {
            const mermaid = await loadMermaid();
            if (!mermaid) continue;

            const mermaidEl = document.createElement("div");
            mermaidEl.className = "mermaid";
            mermaidEl.textContent = item.code;
            item.container.innerHTML = "";
            item.container.appendChild(mermaidEl);

            await mermaid.init(undefined, mermaidEl);
        } catch (error) {
            console.error("[MarkdownRenderer] Mermaid render error:", error);
            item.container.innerHTML = `<pre class="mermaid-error"><code>${item.code}</code></pre>`;
        }
    }

    mermaidProcessing = false;
};

const queueMermaidRender = (code: string, container: HTMLElement) => {
    mermaidRenderQueue.push({ code, container });
    processMermaidQueue();
};

const PLANTUML_SERVER = "https://www.plantuml.com/plantuml/svg";

const plantUmlEncode = (text: string): string => {
    const bytes: number[] = [];
    for (let i = 0; i < text.length; i++) {
        const code = text.charCodeAt(i);
        if (code < 0x80) {
            bytes.push(code);
        } else if (code < 0x800) {
            bytes.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f));
        } else {
            bytes.push(0xe0 | (code >> 12), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));
        }
    }
    let hex = "";
    for (const b of bytes) {
        hex += b.toString(16).padStart(2, "0");
    }
    return "~h" + hex;
};

const getPlantUmlUrl = (code: string): string => {
    return `${PLANTUML_SERVER}/${plantUmlEncode(code)}`;
};

const loadMathJax = (): Promise<void> => {
    if ((window as any).MathJax && (window as any).MathJax.tex2svg) {
        return Promise.resolve();
    }

    if (mathJaxPromise) {
        return mathJaxPromise;
    }

    const existingScript = document.querySelector('script[src*="tex-svg.js"]');
    if (existingScript) {
        mathJaxPromise = new Promise<void>((resolve, reject) => {
            existingScript.addEventListener("load", () => setTimeout(resolve, 100));
            existingScript.addEventListener("error", reject);
        });
        return mathJaxPromise;
    }

    mathJaxPromise = new Promise<void>((resolve, reject) => {
        (window as any).MathJax = {
            tex: {
                inlineMath: [['$', '$']],
                displayMath: [['$$', '$$']],
                processEscapes: true,
            },
            svg: {
                fontCache: 'global',
            },
            startup: {
                ready: () => {
                    (window as any).MathJax.startup.defaultReady();
                    resolve();
                }
            }
        };

        const script = document.createElement('script');
        script.src = 'https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-svg.js';
        script.async = true;
        script.onerror = reject;
        document.head.appendChild(script);
    });

    return mathJaxPromise;
};

const renderMath = (text: string, display: boolean): string => {
    const MathJax = (window as any).MathJax;
    if (!MathJax || !MathJax.tex2svg) {
        return text;
    }

    try {
        const svgElement = MathJax.tex2svg(text, { display });
        return MathJax.startup.adaptor.outerHTML(svgElement);
    } catch (error) {
        console.error('[MarkdownRenderer] MathJax render error:', error);
        return text;
    }
};

const mathExtension = {
    name: "math",
    level: "inline",
    start(src: string) {
        return src.indexOf("$");
    },
    tokenizer(src: string) {
        const inlineMatch = src.match(/^\$([^$\n]+?)\$/);
        if (inlineMatch) {
            return { type: "math", raw: inlineMatch[0], text: inlineMatch[1], displayMode: false };
        }

        const blockMatch = src.match(/^\$\$([\s\S]+?)\$\$/);
        if (blockMatch && blockMatch[1]) {
            return { type: "math", raw: blockMatch[0], text: blockMatch[1]!.trim(), displayMode: true };
        }

        return undefined;
    },
    renderer(token: any) {
        try {
            const renderedMath = renderMath(token.text, token.displayMode);
            if (token.displayMode) {
                return `<div class="mathjax-math-container">${renderedMath}</div>`;
            } else {
                return `<span class="mathjax-math-inline">${renderedMath}</span>`;
            }
        } catch (error) {
            console.error("[MarkdownRenderer] MathJax render error:", error);
            return token.raw;
        }
    },
};

marked.use({
    extensions: [mathExtension],
});

marked.setOptions({
    breaks: true,
    gfm: true,
});

export default defineComponent({
    name: "MarkdownRenderer",
    props: {
        content: {
            type: String,
            required: true,
        },
        isUser: {
            type: Boolean,
            default: false,
        },
        showLoading: {
            type: Boolean,
            default: false,
        },
        isOnlyEmptyTextItem: {
            type: Boolean,
            default: false,
        },
    },
    setup(props) {
        const renderedContent = ref("");
        const isDark = ref(false);
        const markdownRef = ref<HTMLElement | null>(null);
        const backendStore = useBackendStore();

        const checkDarkMode = () => {
            const html = document.documentElement;
            isDark.value = html.classList.contains("dark") || window.matchMedia("(prefers-color-scheme: dark)").matches;
        };

        // Convert markdown to HTML and highlight code blocks
        const renderMarkdown = computed(() => {
            if (!props.isOnlyEmptyTextItem && !props.content) return "";

            try {
                const html = marked.parse(props.content) as string;

                // Apply syntax highlighting to code blocks
                const tempDiv = document.createElement("div");
                tempDiv.innerHTML = html;

                const codeBlocks = tempDiv.querySelectorAll("pre code");

                codeBlocks.forEach((block, index) => {
                    const code = block.textContent || "";
                    const className = block.className || "";
                    const langMatch = className.match(/language-(\w+)/);
                    const lang = langMatch ? langMatch[1] : "plaintext";

                    // Skip highlighting for math language (handled by KaTeX)
                    if (lang === "math") {
                        return;
                    }

                    // Render mermaid diagrams instead of code highlighting
                    if (lang === "mermaid") {
                        const mermaidId = `mermaid-${Date.now()}-${index}`;
                        const mermaidHtml = `
                            <div class="mermaid-container" data-mermaid-code="${encodeURIComponent(code)}">
                                <div class="mermaid-header">
                                    <button class="diagram-view-toggle" onclick="window.toggleMermaidView('${mermaidId}', this)">${backendStore.translate("Source")}</button>
                                    <span></span>
                                    <button class="copy-button" onclick="window.copyMermaid('${mermaidId}', this)" data-tooltip="${backendStore.translate("Copy")}">
                                        <svg class="copy-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
                                            <path fill-opacity="1" fill="currentColor"  transform="translate(1 1)" d="M12 5C13.104569 5 14 5.8954306 14 7L14 12C14 13.104569 13.104569 14 12 14L7 14C5.8954306 14 5 13.104569 5 12L5 7C5 5.8954306 5.8954306 5 7 5L12 5ZM7 0C8.1045694 0 9 0.89543051 9 2L9 4L8 4L8 2C8 1.4871641 7.6139598 1.0644928 7.116621 1.0067277L7 1L2 1C1.4871641 1 1.0644928 1.3860402 1.0067277 1.8833789L1 2L1 7C1 7.512836 1.3860402 7.9355073 1.8863789 7.9932723L2 8L4 8L4 9L2 9C0.89543051 9 0 8.1045694 0 7L0 2C0 0.89543051 0.89543051 0 2 0L7 0Z"/>
                                        </svg>
                                        <svg class="check-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" style="display: none;">
                                            <path fill-opacity="1" fill="currentColor" transform="translate(1.875 2.74996)" d="M10.426653 0.33541855C10.724676 -0.044991303 11.274655 -0.11177851 11.655066 0.18624516C12.006213 0.46134391 12.090132 0.95112652 11.866561 1.3240526L11.804238 1.4146577L4.9492455 10.164658C4.6411924 10.557869 4.0739732 10.609278 3.7009974 10.297988L3.6242123 10.225724L0.23876213 6.6398907C-0.092987508 6.2885051 -0.077069312 5.7347145 0.2743164 5.4029651C0.59867245 5.0967345 1.0954919 5.0867419 1.4309365 5.3633909L1.5112424 5.438519L4.1982522 8.2845383L10.426653 0.33541855Z"/>
                                        </svg>
                                    </button>
                                </div>
                                <div class="mermaid-body">
                                    <div class="mermaid-scroll-wrapper" data-mermaid-id="${mermaidId}" data-mermaid-scroll-init="false">
                                        <div class="mermaid-placeholder"></div>
                                    </div>
                                    <div class="mermaid-scrollbar"><div class="mermaid-scrollbar-thumb"></div></div>
                                    <div class="mermaid-scrollbar-v"><div class="mermaid-scrollbar-v-thumb"></div></div>
                                </div>
                            </div>
                        `;
                        const preElement = block.parentElement;
                        if (preElement) {
                            preElement.outerHTML = mermaidHtml;
                        }
                        return;
                    }

                    // Render PlantUML diagrams as images via server
                    if (lang === "plantuml" || lang === "puml") {
                        const plantumlId = `plantuml-${Date.now()}-${index}`;
                        const svgUrl = getPlantUmlUrl(code);
                        const plantumlHtml = `
                            <div class="plantuml-container" data-plantuml-code="${encodeURIComponent(code)}">
                                <div class="plantuml-header">
                                    <button class="diagram-view-toggle" onclick="window.togglePlantUmlView('${plantumlId}', this)">${backendStore.translate("Source")}</button>
                                    <span></span>
                                    <button class="copy-button" onclick="window.copyPlantUml('${plantumlId}', this)" data-tooltip="${backendStore.translate("Copy")}">
                                        <svg class="copy-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
                                            <path fill-opacity="1" fill="currentColor"  transform="translate(1 1)" d="M12 5C13.104569 5 14 5.8954306 14 7L14 12C14 13.104569 13.104569 14 12 14L7 14C5.8954306 14 5 13.104569 5 12L5 7C5 5.8954306 5.8954306 5 7 5L12 5ZM7 0C8.1045694 0 9 0.89543051 9 2L9 4L8 4L8 2C8 1.4871641 7.6139598 1.0644928 7.116621 1.0067277L7 1L2 1C1.4871641 1 1.0644928 1.3860402 1.0067277 1.8833789L1 2L1 7C1 7.512836 1.3860402 7.9355073 1.8833789 7.9932723L2 8L4 8L4 9L2 9C0.89543051 9 0 8.1045694 0 7L0 2C0 0.89543051 0.89543051 0 2 0L7 0Z"/>
                                        </svg>
                                        <svg class="check-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" style="display: none;">
                                            <path fill-opacity="1" fill="currentColor" transform="translate(1.875 2.74996)" d="M10.426653 0.33541855C10.724676 -0.044991303 11.274655 -0.11177851 11.655066 0.18624516C12.006213 0.46134391 12.090132 0.95112652 11.866561 1.3240526L11.804238 1.4146577L4.9492455 10.164658C4.6411924 10.557869 4.0739732 10.609278 3.7009974 10.297988L3.6242123 10.225724L0.23876213 6.6398907C-0.092987508 6.2885051 -0.077069312 5.7347145 0.2743164 5.4029651C0.59867245 5.0967345 1.0954919 5.0867419 1.4309365 5.3633909L1.5112424 5.438519L4.1982522 8.2845383L10.426653 0.33541855Z"/>
                                        </svg>
                                    </button>
                                </div>
                                <div class="plantuml-body">
                                    <div class="plantuml-scroll-wrapper" data-plantuml-id="${plantumlId}" data-plantuml-scroll-init="false">
                                        <img class="plantuml-img" src="${svgUrl}" alt="PlantUML diagram" />
                                    </div>
                                    <div class="plantuml-scrollbar"><div class="plantuml-scrollbar-thumb"></div></div>
                                    <div class="plantuml-scrollbar-v"><div class="plantuml-scrollbar-v-thumb"></div></div>
                                </div>
                            </div>
                        `;
                        const preElement = block.parentElement;
                        if (preElement) {
                            preElement.outerHTML = plantumlHtml;
                        }
                        return;
                    }

                    try {
                        const highlighted = hljs.highlight(code, { language: lang as string }).value;

                        // Split highlighted code into lines
                        const highlightedLines = highlighted.split("\n");
                        const lineCount = highlightedLines.length;

                        // Create code lines with inline line numbers
                        const codeLinesHtml = highlightedLines
                            .map((line, i) => `<div class="code-line"><span class="code-line-number">${i + 1}</span><span class="code-line-content">${line}</span></div>`)
                            .join("");

                        // Create code block structure with header
                        // Store original code in data-code attribute for copy functionality
                        const encodedCode = encodeURIComponent(code);
                        const blockId = `code-block-${Date.now()}-${index}`;
                        const codeBlockHtml = `
                            <div class="code-block-container" data-block-id="${blockId}" data-line-count="${lineCount}" data-code="${encodedCode}">
                                <div class="code-block-header">
                                    <div class="code-language-wrapper">
                                        <span class="collapse-arrow-wrapper" onclick="window.toggleCodeBlock('${blockId}', this)">
                                            <svg class="collapse-arrow" xmlns="http://www.w3.org/2000/svg" width="12" height="12">
                                                <use xlink:href="#icon-icon_arrow"></use>
                                            </svg>
                                        </span>
                                        <span class="code-language">${lang}</span>
                                    </div>
                                    <button class="copy-button" onclick="window.copyCode('${blockId}', this)" data-tooltip="${backendStore.translate("Copy")}">
                                        <svg class="copy-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
                                            <path fill-opacity="1" fill="currentColor"  transform="translate(1 1)" d="M12 5C13.104569 5 14 5.8954306 14 7L14 12C14 13.104569 13.104569 14 12 14L7 14C5.8954306 14 5 13.104569 5 12L5 7C5 5.8954306 5.8954306 5 7 5L12 5ZM7 0C8.1045694 0 9 0.89543051 9 2L9 4L8 4L8 2C8 1.4871641 7.6139598 1.0644928 7.116621 1.0067277L7 1L2 1C1.4871641 1 1.0644928 1.3860402 1.0067277 1.8833789L1 2L1 7C1 7.512836 1.3860402 7.9355073 1.8833789 7.9932723L2 8L4 8L4 9L2 9C0.89543051 9 0 8.1045694 0 7L0 2C0 0.89543051 0.89543051 0 2 0L7 0Z"/>
                                        </svg>
                                        <svg class="check-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" style="display: none;">
                                            <path fill-opacity="1" fill="currentColor" transform="translate(1.875 2.74996)" d="M10.426653 0.33541855C10.724676 -0.044991303 11.274655 -0.11177851 11.655066 0.18624516C12.006213 0.46134391 12.090132 0.95112652 11.866561 1.3240526L11.804238 1.4146577L4.9492455 10.164658C4.6411924 10.557869 4.0739732 10.609278 3.7009974 10.297988L3.6242123 10.225724L0.23876213 6.6398907C-0.092987508 6.2885051 -0.077069312 5.7347145 0.2743164 5.4029651C0.59867245 5.0967345 1.0954919 5.0867419 1.4309365 5.3633909L1.5112424 5.438519L4.1982522 8.2845383L10.426653 0.33541855Z"/>
                                        </svg>
                                    </button>
                                </div>
                                <div class="code-block-body">
                                    <div class="code-content hljs language-${lang}">${codeLinesHtml}</div>
                                </div>
                                <div class="code-block-collapsed" style="display: none;">
                                    <span class="collapsed-text">${lineCount} ${backendStore.translate("lines of code collapsed")}</span>
                                    <span class="collapsed-expand-link" onclick="window.toggleCodeBlock('${blockId}', null)">${backendStore.translate("Expand")}</span>
                                </div>
                            </div>
                        `;

                        // Replace the parent pre element with our new structure
                        const preElement = block.parentElement;
                        if (preElement) {
                            preElement.outerHTML = codeBlockHtml;
                        }
                    } catch (error) {
                        // Only log error if it's a real language, not if it's our custom marker
                        if (!className.includes("loading-dot") && !className.includes("message-loading-indicator")) {
                            console.error(`[MarkdownRenderer] ${lang} language highlight failed:`, error);
                        }
                    }
                });

                let finalHtml = tempDiv.innerHTML;

                // Remove # symbols from heading text while keeping the heading tags
                finalHtml = finalHtml.replace(/<h(\d+)>(.*?)<\/h\1>/g, (match, level, content) => {
                    // Remove # symbols from the beginning of the content
                    const cleanContent = content.replace(/^#+\s*/, "").trim();
                    return `<h${level}>${cleanContent}</h${level}>`;
                });

                // Wrap tables with container and header
                if (finalHtml.includes("<table>")) {
                    finalHtml = finalHtml.replace(/<table>/g, '<table class="marked-table">');

                    // Process tables to add wrapper and header
                    const tempDiv3 = document.createElement("div");
                    tempDiv3.innerHTML = finalHtml;

                    const tables = tempDiv3.querySelectorAll("table.marked-table");
                    tables.forEach((table, index) => {
                        const tableId = `table-${Date.now()}-${index}`;
                        const tableHtml = `
                            <div class="table-container" data-table-id="${tableId}">
                                <div class="table-header">
                                    <button class="copy-button" onclick="window.copyTable('${tableId}', this)" data-tooltip="${backendStore.translate("Copy")}">
                                        <svg class="copy-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16">
                                            <path fill-opacity="1" fill="currentColor"  transform="translate(1 1)" d="M12 5C13.104569 5 14 5.8954306 14 7L14 12C14 13.104569 13.104569 14 12 14L7 14C5.8954306 14 5 13.104569 5 12L5 7C5 5.8954306 5.8954306 5 7 5L12 5ZM7 0C8.1045694 0 9 0.89543051 9 2L9 4L8 4L8 2C8 1.4871641 7.6139598 1.0644928 7.116621 1.0067277L7 1L2 1C1.4871641 1 1.0644928 1.3860402 1.0067277 1.8833789L1 2L1 7C1 7.512836 1.3860402 7.9355073 1.8833789 7.9932723L2 8L4 8L4 9L2 9C0.89543051 9 0 8.1045694 0 7L0 2C0 0.89543051 0.89543051 0 2 0L7 0Z"/>
                                        </svg>
                                        <svg class="check-icon" xmlns="http://www.w3.org/2000/svg" width="16" height="16" style="display: none;">
                                            <path fill-opacity="1" fill="currentColor" transform="translate(1.875 2.74996)" d="M10.426653 0.33541855C10.724676 -0.044991303 11.274655 -0.11177851 11.655066 0.18624516C12.006213 0.46134391 12.090132 0.95112652 11.866561 1.3240526L11.804238 1.4146577L4.9492455 10.164658C4.6411924 10.557869 4.0739732 10.609278 3.7009974 10.297988L3.6242123 10.225724L0.23876213 6.6398907C-0.092987508 6.2885051 -0.077069312 5.7347145 0.2743164 5.4029651C0.59867245 5.0967345 1.0954919 5.0867419 1.4309365 5.3633909L1.5112424 5.438519L4.1982522 8.2845383L10.426653 0.33541855Z"/>
                                        </svg>
                                    </button>
                                </div>
                                <div class="table-body">
                                    <div class="table-wrapper" data-table-wrapper-id="${tableId}">${table.outerHTML}</div>
                                    <div class="table-scrollbar" data-table-scrollbar-id="${tableId}">
                                        <div class="table-scrollbar-thumb"></div>
                                    </div>
                                    <div class="table-corner-mask"></div>
                                </div>
                            </div>
                        `;
                        table.outerHTML = tableHtml;
                    });

                    finalHtml = tempDiv3.innerHTML;
                }

                // Add margin to pre tags and ensure hljs class
                if (finalHtml.includes('<pre><code class="')) {
                    finalHtml = finalHtml.replace(
                        /<pre><code class="/g,
                        '<pre style="margin-top:20px;margin-bottom: 20px;"><code class="hljs ',
                    );
                }

                // If streaming, append loading animation marker inline
                // Insert it at the end of the last text node to ensure it follows the last character
                // Only show loading if NOT currently rendering inside unclosed code blocks, tables, or horizontal rules
                if (props.showLoading) {
                    // Check if content ends with an unclosed code block
                    // Match the last occurrence of ``` and check if it's closed
                    const codeBlockMatches = props.content.split("```");
                    const hasUnclosedCodeBlock = codeBlockMatches.length % 2 === 0;

                    // Check if content ends with an unclosed table (<table> tag without closing)
                    // Find the last occurrence of <table> and check if there's a closing </table> after it
                    const lastTableStart = props.content.lastIndexOf("<table>");
                    const lastTableClose = props.content.lastIndexOf("</table>");
                    const hasUnclosedTable = lastTableStart > lastTableClose;

                    // Check if content is currently outputting a horizontal rule
                    // A horizontal rule in markdown is a line containing only --- or ***
                    const lines = props.content.split("\n");
                    const lastLine = lines[lines.length - 1]?.trim() || "";
                    
                    // Check if the last line is a horizontal rule pattern (exactly 3+ dashes or asterisks)
                    const isLastLineHr = /^-{3,}\s*$/.test(lastLine) || /^\*{3,}\s*$/.test(lastLine);
                    
                    // Check if the last line is an incomplete horizontal rule pattern
                    const isIncompleteHrPattern = /^-{1,2}\s*$/.test(lastLine) || /^\*{1,2}\s*$/.test(lastLine);
                    
                    // Check if content ends with HTML hr tag
                    const endsWithHrTag = props.content.trim().endsWith("<hr>") || props.content.trim().endsWith("<hr ");

                    // Only append loading indicator if there's no unclosed code block, unclosed table, or horizontal rule
                    if (!hasUnclosedCodeBlock && !hasUnclosedTable && !isLastLineHr && !isIncompleteHrPattern && !endsWithHrTag) {
                        // Choose loading indicator based on isOnlyEmptyTextItem
                        const loadingHtml = props.isOnlyEmptyTextItem
                            ? '<span class="waiting-loading-indicator"><span class="waiting-dot"></span><span class="waiting-dot"></span><span class="waiting-dot"></span></span>'
                            : '<span class="message-loading-indicator"><span class="loading-dot"></span><span class="loading-dot"></span><span class="loading-dot"></span></span>';

                        // Create a temporary div to manipulate the HTML
                        const tempDiv2 = document.createElement("div");
                        tempDiv2.innerHTML = finalHtml;

                        // Find the last paragraph or inline element
                        const paragraphs = tempDiv2.querySelectorAll("p, li, span");
                        let lastInlineElement: Element | null = null;

                        for (let i = 0; i < paragraphs.length; i++) {
                            const elem = paragraphs[i];
                            if (!elem) continue;

                            const parentTag = elem.parentElement?.tagName.toLowerCase();
                            // Skip if it's inside a code block or table
                            if (
                                parentTag !== "pre" &&
                                parentTag !== "code" &&
                                parentTag !== "td" &&
                                parentTag !== "th" &&
                                !elem.closest("pre") &&
                                !elem.closest("code") &&
                                !elem.closest("table")
                            ) {
                                lastInlineElement = elem;
                            }
                        }

                        if (lastInlineElement) {
                            // Append loading indicator to the last inline element
                            lastInlineElement.innerHTML += loadingHtml;
                            finalHtml = tempDiv2.innerHTML;
                        } else {
                            // Fallback: append to the end of the content
                            finalHtml += loadingHtml;
                        }
                    }
                }

                return finalHtml;
            } catch (error) {
                console.error("[MarkdownRenderer] Markdown parse error:", error);
                return props.content;
            }
        });

        // Copy code function exposed to window
        const copyCode = (blockId: string, button: HTMLButtonElement) => {
            const container = document.querySelector(`[data-block-id="${blockId}"]`) as HTMLElement;
            if (!container) return;

            // Get original code from data-code attribute (without line numbers)
            const encodedCode = container.getAttribute("data-code") || "";
            const codeContent = decodeURIComponent(encodedCode);

            // Fallback for browsers without clipboard API
            if (!navigator.clipboard) {
                // Create a textarea to copy text
                const textarea = document.createElement("textarea");
                textarea.value = codeContent;
                textarea.style.position = "fixed";
                textarea.style.opacity = "0";
                document.body.appendChild(textarea);
                textarea.select();

                try {
                    const successful = document.execCommand("copy");
                    document.body.removeChild(textarea);

                    if (successful) {
                    // Show check icon
                    const copyIcon = button.querySelector(".copy-icon") as HTMLElement;
                    const checkIcon = button.querySelector(".check-icon") as HTMLElement;

                    if (copyIcon) copyIcon.style.display = "none";
                    if (checkIcon) checkIcon.style.display = "inline-block";
                    button.setAttribute("data-tooltip", backendStore.translate("Copied"));
                    updateTooltipContent(button, backendStore.translate("Copied"));

                    // Reset after 2 seconds
                    setTimeout(() => {
                        if (copyIcon) copyIcon.style.display = "inline-block";
                        if (checkIcon) checkIcon.style.display = "none";
                        button.setAttribute("data-tooltip", backendStore.translate("Copy"));
                        updateTooltipContent(button, backendStore.translate("Copy"));
                    }, 2000);
                }
                } catch (err) {
                    console.error("[MarkdownRenderer] Copy failed:", err);
                    document.body.removeChild(textarea);
                }
                return;
            }

            // Use clipboard API for modern browsers
            navigator.clipboard
                .writeText(codeContent)
                .then(() => {
                    // Show check icon
                    const copyIcon = button.querySelector(".copy-icon") as HTMLElement;
                    const checkIcon = button.querySelector(".check-icon") as HTMLElement;

                    if (copyIcon) copyIcon.style.display = "none";
                    if (checkIcon) checkIcon.style.display = "inline-block";
                    button.setAttribute("data-tooltip", backendStore.translate("Copied"));
                    updateTooltipContent(button, backendStore.translate("Copied"));

                    // Reset after 2 seconds
                    setTimeout(() => {
                        if (copyIcon) copyIcon.style.display = "inline-block";
                        if (checkIcon) checkIcon.style.display = "none";
                        button.setAttribute("data-tooltip", backendStore.translate("Copy"));
                        updateTooltipContent(button, backendStore.translate("Copy"));
                    }, 2000);
                })
                .catch((error) => {
                    console.error("[MarkdownRenderer] Copy failed:", error);
                });
        };

        // Toggle code block collapse/expand
        const toggleCodeBlock = (blockId: string, arrowWrapper: HTMLElement | null) => {
            const container = document.querySelector(`[data-block-id="${blockId}"]`) as HTMLElement;
            if (!container) return;

            const body = container.querySelector(".code-block-body") as HTMLElement;
            const collapsed = container.querySelector(".code-block-collapsed") as HTMLElement;
            const arrowElement = (arrowWrapper || container.querySelector(".collapse-arrow-wrapper")) as HTMLElement;
            const isCollapsed = body.style.display === "none";

            if (isCollapsed) {
                // Expand
                body.style.display = "flex";
                collapsed.style.display = "none";
                // Remove collapsed class to rotate arrow
                if (arrowElement) {
                    arrowElement.classList.remove("collapse-arrow-wrapper--collapsed");
                }
            } else {
                // Collapse
                body.style.display = "none";
                collapsed.style.display = "flex";
                // Add collapsed class to rotate arrow
                if (arrowElement) {
                    arrowElement.classList.add("collapse-arrow-wrapper--collapsed");
                }
            }
        };

        // Initialize tooltips for all copy buttons
        const initTooltips = () => {
            // No initialization needed - tooltips work via CSS with data-tooltip attribute
        };

        // Update tooltip content dynamically
        const updateTooltipContent = (button: HTMLElement, newText: string) => {
            // Update data attribute - CSS will handle the rest
            button.setAttribute('data-tooltip', newText);
        };

        // Copy table function exposed to window
        const copyTable = (tableId: string, button: HTMLButtonElement) => {
            const container = document.querySelector(`[data-table-id="${tableId}"]`) as HTMLElement;
            if (!container) return;

            const tableElement = container.querySelector("table") as HTMLElement;
            if (!tableElement) return;

            // Extract table data as formatted text
            const rows = tableElement.querySelectorAll("tr");
            let tableText = "";

            rows.forEach((row) => {
                const cells = row.querySelectorAll("th, td");
                const rowData = Array.from(cells)
                    .map((cell) => cell.textContent || "")
                    .join("\t");
                tableText += rowData + "\n";
            });

            // Fallback for browsers without clipboard API
            if (!navigator.clipboard) {
                const textarea = document.createElement("textarea");
                textarea.value = tableText;
                textarea.style.position = "fixed";
                textarea.style.opacity = "0";
                document.body.appendChild(textarea);
                textarea.select();

                try {
                    const successful = document.execCommand("copy");
                    document.body.removeChild(textarea);

                    if (successful) {
                        const copyIcon = button.querySelector(".copy-icon") as HTMLElement;
                        const checkIcon = button.querySelector(".check-icon") as HTMLElement;

                        if (copyIcon) copyIcon.style.display = "none";
                        if (checkIcon) checkIcon.style.display = "inline-block";
                        button.setAttribute("data-tooltip", backendStore.translate("Copied"));
                        updateTooltipContent(button, backendStore.translate("Copied"));

                        setTimeout(() => {
                            if (copyIcon) copyIcon.style.display = "inline-block";
                            if (checkIcon) checkIcon.style.display = "none";
                            button.setAttribute("data-tooltip", backendStore.translate("Copy"));
                            updateTooltipContent(button, backendStore.translate("Copy"));
                        }, 2000);
                    }
                } catch (err) {
                    console.error("[MarkdownRenderer] Copy table failed:", err);
                    document.body.removeChild(textarea);
                }
                return;
            }

            // Use clipboard API for modern browsers
            navigator.clipboard
                .writeText(tableText)
                .then(() => {
                    const copyIcon = button.querySelector(".copy-icon") as HTMLElement;
                    const checkIcon = button.querySelector(".check-icon") as HTMLElement;

                    if (copyIcon) copyIcon.style.display = "none";
                    if (checkIcon) checkIcon.style.display = "inline-block";
                    button.setAttribute("data-tooltip", backendStore.translate("Copied"));
                    updateTooltipContent(button, backendStore.translate("Copied"));

                    setTimeout(() => {
                        if (copyIcon) copyIcon.style.display = "inline-block";
                        if (checkIcon) checkIcon.style.display = "none";
                        button.setAttribute("data-tooltip", backendStore.translate("Copy"));
                        updateTooltipContent(button, backendStore.translate("Copy"));
                    }, 2000);
                })
                .catch((error) => {
                    console.error("[MarkdownRenderer] Copy table failed:", error);
                });
        };


        // Calculate dynamic unit U based on base font size
        const calculateUnitU = () => {
            const baseFontSize = parseFloat(getComputedStyle(document.documentElement).getPropertyValue('--uosai-font-size-base')) || 1;
            const fontSizePx = baseFontSize * 16; // Convert rem to px (assuming 1rem = 16px)
            const U = Math.floor(fontSizePx / 4);
            document.documentElement.style.setProperty('--U', `${U}px`);
        };

        // Watch for content changes to initialize table scroll and code block sticky observer
        watch(() => props.content, () => {
            nextTick(() => {
                setTimeout(() => {
                    initMermaidDiagrams();
                    initMermaidScroll();
                    initPlantUmlScroll();
                    initCodeBlockStickyObserver();
                    initTableScroll();
                    initTooltips();
                }, 50);
            });
        });

        // Initialize mermaid diagrams
        const initMermaidDiagrams = () => {
            const root = markdownRef.value;
            if (!root) return;

            const containers = root.querySelectorAll(".mermaid-container");
            if (containers.length === 0) return;

            containers.forEach((container) => {
                const element = container as HTMLElement;
                if (element.dataset.mermaidRendered === "true") return;

                const placeholder = element.querySelector(".mermaid-placeholder");
                if (!placeholder) return;

                const code = decodeURIComponent(element.dataset.mermaidCode || "");
                element.dataset.mermaidRendered = "true";
                queueMermaidRender(code, placeholder.parentElement as HTMLElement);

                // Watch for SVG insertion to init scrollbars
                const scrollWrapper = element.querySelector('.mermaid-scroll-wrapper') as HTMLElement;
                if (scrollWrapper) {
                    const mo = new MutationObserver(() => {
                        if (scrollWrapper.querySelector('svg')) {
                            mo.disconnect();
                            setTimeout(() => initMermaidScroll(), 50);
                        }
                    });
                    mo.observe(scrollWrapper, { childList: true, subtree: true });
                }
            });
        };

        // Initialize mermaid scrollbars (horizontal + vertical)
        const initMermaidScroll = () => {
            const scrollWrappers = document.querySelectorAll('.markdown-content .mermaid-scroll-wrapper');

            scrollWrappers.forEach((wrapper) => {
                const element = wrapper as HTMLElement;
                if (element.dataset.mermaidScrollInit === "true") return;

                const hasContent = element.querySelector('svg') || element.querySelector('.mermaid-error');
                if (!hasContent) return;

                element.dataset.mermaidScrollInit = "true";

                const body = element.closest('.mermaid-body') as HTMLElement;
                const hScrollbar = body?.querySelector('.mermaid-scrollbar') as HTMLElement;
                const hThumb = hScrollbar?.querySelector('.mermaid-scrollbar-thumb') as HTMLElement;
                const vScrollbar = body?.querySelector('.mermaid-scrollbar-v') as HTMLElement;
                const vThumb = vScrollbar?.querySelector('.mermaid-scrollbar-v-thumb') as HTMLElement;

                // Horizontal scrollbar
                if (hScrollbar && hThumb) {
                    const checkHScroll = () => {
                        if (element.scrollWidth > element.clientWidth) {
                            hScrollbar.classList.add('visible');
                            updateMermaidHScrollbar(element, hThumb);
                        } else {
                            hScrollbar.classList.remove('visible');
                        }
                    };

                    const resizeObserver = new ResizeObserver(() => checkHScroll());
                    resizeObserver.observe(element);

                    let scrollTimeout: number | null = null;
                    element.addEventListener('scroll', () => {
                        element.classList.add('scrolling');
                        updateMermaidHScrollbar(element, hThumb);
                        updateMermaidVScrollbar(element, vThumb);
                        if (scrollTimeout) clearTimeout(scrollTimeout);
                        scrollTimeout = window.setTimeout(() => element.classList.remove('scrolling'), 1000);
                    });

                    // H thumb drag
                    let hDragging = false;
                    let hDragStartX = 0;
                    let hDragStartScrollLeft = 0;

                    const onHThumbMouseDown = (e: MouseEvent) => {
                        e.preventDefault();
                        e.stopPropagation();
                        hDragging = true;
                        hDragStartX = e.clientX;
                        hDragStartScrollLeft = element.scrollLeft;
                        hThumb.classList.add('dragging');
                        element.style.scrollBehavior = 'auto';
                        document.addEventListener('mousemove', onHThumbMouseMove);
                        document.addEventListener('mouseup', onHThumbMouseUp);
                    };

                    const onHThumbMouseMove = (e: MouseEvent) => {
                        if (!hDragging) return;
                        const maxScroll = element.scrollWidth - element.clientWidth;
                        const trackWidth = element.clientWidth - 32;
                        const thumbWidth = Math.max(20, (element.clientWidth / element.scrollWidth) * trackWidth);
                        const trackTravel = trackWidth - thumbWidth;
                        const currentPos = ((hDragStartScrollLeft / maxScroll) * trackTravel) + (e.clientX - hDragStartX);
                        const clamped = Math.max(0, Math.min(trackTravel, currentPos));
                        element.scrollLeft = trackTravel > 0 ? (clamped / trackTravel) * maxScroll : 0;
                    };

                    const onHThumbMouseUp = () => {
                        hDragging = false;
                        hThumb.classList.remove('dragging');
                        element.style.scrollBehavior = '';
                        document.removeEventListener('mousemove', onHThumbMouseMove);
                        document.removeEventListener('mouseup', onHThumbMouseUp);
                    };

                    hThumb.addEventListener('mousedown', onHThumbMouseDown);

                    hScrollbar.addEventListener('click', (e: MouseEvent) => {
                        if (e.target === hThumb) return;
                        const rect = hScrollbar.getBoundingClientRect();
                        const clickX = e.clientX - rect.left;
                        const maxScroll = element.scrollWidth - element.clientWidth;
                        const trackWidth = element.clientWidth - 32;
                        const thumbWidth = Math.max(20, (element.clientWidth / element.scrollWidth) * trackWidth);
                        const trackTravel = trackWidth - thumbWidth;
                        element.scrollLeft = trackTravel > 0 ? (clickX / trackWidth) * maxScroll : 0;
                    });

                    checkHScroll();
                }

                // Vertical scrollbar
                if (vScrollbar && vThumb) {
                    const checkVScroll = () => {
                        if (element.scrollHeight > element.clientHeight) {
                            vScrollbar.classList.add('visible');
                            updateMermaidVScrollbar(element, vThumb);
                        } else {
                            vScrollbar.classList.remove('visible');
                        }
                    };

                    const resizeObserver2 = new ResizeObserver(() => checkVScroll());
                    resizeObserver2.observe(element);

                    // V thumb drag
                    let vDragging = false;
                    let vDragStartY = 0;
                    let vDragStartScrollTop = 0;

                    const onVThumbMouseDown = (e: MouseEvent) => {
                        e.preventDefault();
                        e.stopPropagation();
                        vDragging = true;
                        vDragStartY = e.clientY;
                        vDragStartScrollTop = element.scrollTop;
                        vThumb.classList.add('dragging');
                        element.style.scrollBehavior = 'auto';
                        document.addEventListener('mousemove', onVThumbMouseMove);
                        document.addEventListener('mouseup', onVThumbMouseUp);
                    };

                    const onVThumbMouseMove = (e: MouseEvent) => {
                        if (!vDragging) return;
                        const maxScroll = element.scrollHeight - element.clientHeight;
                        const trackHeight = vScrollbar.clientHeight;
                        const thumbHeight = Math.max(20, (element.clientHeight / element.scrollHeight) * trackHeight);
                        const trackTravel = trackHeight - thumbHeight;
                        const currentPos = ((vDragStartScrollTop / maxScroll) * trackTravel) + (e.clientY - vDragStartY);
                        const clamped = Math.max(0, Math.min(trackTravel, currentPos));
                        element.scrollTop = trackTravel > 0 ? (clamped / trackTravel) * maxScroll : 0;
                    };

                    const onVThumbMouseUp = () => {
                        vDragging = false;
                        vThumb.classList.remove('dragging');
                        element.style.scrollBehavior = '';
                        document.removeEventListener('mousemove', onVThumbMouseMove);
                        document.removeEventListener('mouseup', onVThumbMouseUp);
                    };

                    vThumb.addEventListener('mousedown', onVThumbMouseDown);

                    vScrollbar.addEventListener('click', (e: MouseEvent) => {
                        if (e.target === vThumb) return;
                        const rect = vScrollbar.getBoundingClientRect();
                        const clickY = e.clientY - rect.top;
                        const maxScroll = element.scrollHeight - element.clientHeight;
                        const trackHeight = vScrollbar.clientHeight;
                        const thumbHeight = Math.max(20, (element.clientHeight / element.scrollHeight) * trackHeight);
                        const trackTravel = trackHeight - thumbHeight;
                        element.scrollTop = trackTravel > 0 ? (clickY / trackHeight) * maxScroll : 0;
                    });

                    checkVScroll();
                }
            });
        };

        const updateMermaidHScrollbar = (body: HTMLElement, thumb: HTMLElement) => {
            if (!thumb) return;
            const maxScroll = body.scrollWidth - body.clientWidth;
            if (maxScroll <= 0) return;
            const trackWidth = body.clientWidth - 32;
            const thumbWidth = Math.max(20, (body.clientWidth / body.scrollWidth) * trackWidth);
            const thumbPosition = (body.scrollLeft / maxScroll) * (trackWidth - thumbWidth);
            thumb.style.width = `${thumbWidth}px`;
            thumb.style.left = `${thumbPosition}px`;
        };

        const updateMermaidVScrollbar = (body: HTMLElement, thumb: HTMLElement) => {
            if (!thumb) return;
            const scrollbar = thumb.parentElement as HTMLElement;
            if (!scrollbar) return;
            const maxScroll = body.scrollHeight - body.clientHeight;
            if (maxScroll <= 0) return;
            const trackHeight = scrollbar.clientHeight;
            const thumbHeight = Math.max(20, (body.clientHeight / body.scrollHeight) * trackHeight);
            const thumbPosition = (body.scrollTop / maxScroll) * (trackHeight - thumbHeight);
            thumb.style.height = `${thumbHeight}px`;
            thumb.style.top = `${thumbPosition}px`;
        };

        const initPlantUmlScroll = () => {
            const scrollWrappers = document.querySelectorAll('.markdown-content .plantuml-scroll-wrapper');

            scrollWrappers.forEach((wrapper) => {
                const element = wrapper as HTMLElement;
                if (element.dataset.plantumlScrollInit === "true") return;
                element.dataset.plantumlScrollInit = "true";

                const img = element.querySelector('.plantuml-img') as HTMLImageElement;
                const initScroll = () => {
                    const body = element.closest('.plantuml-body') as HTMLElement;
                    const hScrollbar = body?.querySelector('.plantuml-scrollbar') as HTMLElement;
                    const hThumb = hScrollbar?.querySelector('.plantuml-scrollbar-thumb') as HTMLElement;
                    const vScrollbar = body?.querySelector('.plantuml-scrollbar-v') as HTMLElement;
                    const vThumb = vScrollbar?.querySelector('.plantuml-scrollbar-v-thumb') as HTMLElement;

                    if (hScrollbar && hThumb) {
                        const checkHScroll = () => {
                            if (element.scrollWidth > element.clientWidth) {
                                hScrollbar.classList.add('visible');
                                updateMermaidHScrollbar(element, hThumb);
                            } else {
                                hScrollbar.classList.remove('visible');
                            }
                        };

                        new ResizeObserver(() => checkHScroll()).observe(element);

                        element.addEventListener('scroll', () => {
                            updateMermaidHScrollbar(element, hThumb);
                            updateMermaidVScrollbar(element, vThumb);
                        });

                        let hDragging = false, hDragStartX = 0, hDragStartScrollLeft = 0;
                        hThumb.addEventListener('mousedown', (e) => {
                            e.preventDefault(); e.stopPropagation();
                            hDragging = true; hDragStartX = e.clientX; hDragStartScrollLeft = element.scrollLeft;
                            hThumb.classList.add('dragging');
                            element.style.scrollBehavior = 'auto';
                            const onMove = (e: MouseEvent) => {
                                if (!hDragging) return;
                                const maxScroll = element.scrollWidth - element.clientWidth;
                                const trackWidth = element.clientWidth - 32;
                                const thumbWidth = Math.max(20, (element.clientWidth / element.scrollWidth) * trackWidth);
                                const trackTravel = trackWidth - thumbWidth;
                                const clamped = Math.max(0, Math.min(trackTravel, ((hDragStartScrollLeft / maxScroll) * trackTravel) + (e.clientX - hDragStartX)));
                                element.scrollLeft = trackTravel > 0 ? (clamped / trackTravel) * maxScroll : 0;
                            };
                            const onUp = () => {
                                hDragging = false; hThumb.classList.remove('dragging');
                                element.style.scrollBehavior = '';
                                document.removeEventListener('mousemove', onMove);
                                document.removeEventListener('mouseup', onUp);
                            };
                            document.addEventListener('mousemove', onMove);
                            document.addEventListener('mouseup', onUp);
                        });

                        hScrollbar.addEventListener('click', (e: MouseEvent) => {
                            if (e.target === hThumb) return;
                            const clickX = e.clientX - hScrollbar.getBoundingClientRect().left;
                            const maxScroll = element.scrollWidth - element.clientWidth;
                            const trackWidth = element.clientWidth - 32;
                            const thumbWidth = Math.max(20, (element.clientWidth / element.scrollWidth) * trackWidth);
                            const trackTravel = trackWidth - thumbWidth;
                            element.scrollLeft = trackTravel > 0 ? (clickX / trackWidth) * maxScroll : 0;
                        });

                        checkHScroll();
                    }

                    if (vScrollbar && vThumb) {
                        const checkVScroll = () => {
                            if (element.scrollHeight > element.clientHeight) {
                                vScrollbar.classList.add('visible');
                                updateMermaidVScrollbar(element, vThumb);
                            } else {
                                vScrollbar.classList.remove('visible');
                            }
                        };

                        new ResizeObserver(() => checkVScroll()).observe(element);

                        element.addEventListener('scroll', () => {
                            updateMermaidVScrollbar(element, vThumb);
                        });

                        let vDragging = false, vDragStartY = 0, vDragStartScrollTop = 0;
                        vThumb.addEventListener('mousedown', (e) => {
                            e.preventDefault(); e.stopPropagation();
                            vDragging = true; vDragStartY = e.clientY; vDragStartScrollTop = element.scrollTop;
                            vThumb.classList.add('dragging');
                            element.style.scrollBehavior = 'auto';
                            const onMove = (e: MouseEvent) => {
                                if (!vDragging) return;
                                const maxScroll = element.scrollHeight - element.clientHeight;
                                const trackHeight = vScrollbar.clientHeight;
                                const thumbHeight = Math.max(20, (element.clientHeight / element.scrollHeight) * trackHeight);
                                const trackTravel = trackHeight - thumbHeight;
                                const clamped = Math.max(0, Math.min(trackTravel, ((vDragStartScrollTop / maxScroll) * trackTravel) + (e.clientY - vDragStartY)));
                                element.scrollTop = trackTravel > 0 ? (clamped / trackTravel) * maxScroll : 0;
                            };
                            const onUp = () => {
                                vDragging = false; vThumb.classList.remove('dragging');
                                element.style.scrollBehavior = '';
                                document.removeEventListener('mousemove', onMove);
                                document.removeEventListener('mouseup', onUp);
                            };
                            document.addEventListener('mousemove', onMove);
                            document.addEventListener('mouseup', onUp);
                        });

                        vScrollbar.addEventListener('click', (e: MouseEvent) => {
                            if (e.target === vThumb) return;
                            const clickY = e.clientY - vScrollbar.getBoundingClientRect().top;
                            const maxScroll = element.scrollHeight - element.clientHeight;
                            const trackHeight = vScrollbar.clientHeight;
                            const thumbHeight = Math.max(20, (element.clientHeight / element.scrollHeight) * trackHeight);
                            const trackTravel = trackHeight - thumbHeight;
                            element.scrollTop = trackTravel > 0 ? (clickY / trackHeight) * maxScroll : 0;
                        });

                        checkVScroll();
                    }
                };

                if (img && img.complete) {
                    initScroll();
                } else if (img) {
                    img.addEventListener('load', initScroll);
                }
            });
        };

        // Toggle mermaid view: preview <-> source code
        const toggleMermaidView = (mermaidId: string, button: HTMLButtonElement) => {
            const container = document.querySelector(`[data-mermaid-id="${mermaidId}"]`)?.closest('.mermaid-container') as HTMLElement;
            if (!container) return;
            const wrapper = container.querySelector('.mermaid-scroll-wrapper') as HTMLElement;
            if (!wrapper) return;

            const isSourceMode = container.classList.toggle('source-mode');
            const code = decodeURIComponent(container.dataset.mermaidCode || "");

            if (isSourceMode) {
                wrapper.innerHTML = `<pre class="diagram-source-code"><code>${code.replace(/</g, '&lt;').replace(/>/g, '&gt;')}</code></pre>`;
                button.textContent = backendStore.translate("Preview");
            } else {
                wrapper.innerHTML = '<div class="mermaid-placeholder"></div>';
                container.dataset.mermaidRendered = "false";
                container.dataset.mermaidScrollInit = "false";
                const scrollWrapper = container.querySelector('.mermaid-scroll-wrapper') as HTMLElement;
                if (scrollWrapper) {
                    const mo = new MutationObserver(() => {
                        if (scrollWrapper.querySelector('svg')) {
                            mo.disconnect();
                            setTimeout(() => initMermaidScroll(), 50);
                        }
                    });
                    mo.observe(scrollWrapper, { childList: true, subtree: true });
                }
                initMermaidDiagrams();
                button.textContent = backendStore.translate("Source");
            }
        };

        // Toggle plantuml view: preview <-> source code
        const togglePlantUmlView = (plantumlId: string, button: HTMLButtonElement) => {
            const container = document.querySelector(`[data-plantuml-id="${plantumlId}"]`)?.closest('.plantuml-container') as HTMLElement;
            if (!container) return;
            const wrapper = container.querySelector('.plantuml-scroll-wrapper') as HTMLElement;
            if (!wrapper) return;

            const isSourceMode = container.classList.toggle('source-mode');
            const code = decodeURIComponent(container.dataset.plantumlCode || "");
            const svgUrl = getPlantUmlUrl(code);

            if (isSourceMode) {
                wrapper.innerHTML = `<pre class="diagram-source-code"><code>${code.replace(/</g, '&lt;').replace(/>/g, '&gt;')}</code></pre>`;
                button.textContent = backendStore.translate("Preview");
            } else {
                wrapper.innerHTML = `<img class="plantuml-img" src="${svgUrl}" alt="PlantUML diagram" />`;
                container.dataset.plantumlScrollInit = "false";
                initPlantUmlScroll();
                button.textContent = backendStore.translate("Source");
            }
        };

        // Copy mermaid source code function exposed to window
        const copyMermaid = (mermaidId: string, button: HTMLButtonElement) => {
            const body = document.querySelector(`[data-mermaid-id="${mermaidId}"]`) as HTMLElement;
            if (!body) return;

            const container = body.closest(".mermaid-container") as HTMLElement;
            if (!container) return;

            const code = decodeURIComponent(container.dataset.mermaidCode || "");

            const doCopy = () => {
                const copyIcon = button.querySelector(".copy-icon") as HTMLElement;
                const checkIcon = button.querySelector(".check-icon") as HTMLElement;

                if (copyIcon) copyIcon.style.display = "none";
                if (checkIcon) checkIcon.style.display = "inline-block";
                button.setAttribute("data-tooltip", backendStore.translate("Copied"));
                updateTooltipContent(button, backendStore.translate("Copied"));

                setTimeout(() => {
                    if (copyIcon) copyIcon.style.display = "inline-block";
                    if (checkIcon) checkIcon.style.display = "none";
                    button.setAttribute("data-tooltip", backendStore.translate("Copy"));
                    updateTooltipContent(button, backendStore.translate("Copy"));
                }, 2000);
            };

            if (!navigator.clipboard) {
                const textarea = document.createElement("textarea");
                textarea.value = code;
                textarea.style.position = "fixed";
                textarea.style.opacity = "0";
                document.body.appendChild(textarea);
                textarea.select();
                try {
                    if (document.execCommand("copy")) {
                        document.body.removeChild(textarea);
                        doCopy();
                    }
                } catch (err) {
                    console.error("[MarkdownRenderer] Copy mermaid failed:", err);
                    document.body.removeChild(textarea);
                }
                return;
            }

            navigator.clipboard.writeText(code).then(doCopy).catch((error) => {
                console.error("[MarkdownRenderer] Copy mermaid failed:", error);
            });
        };

        // Copy plantuml source code function exposed to window
        const copyPlantUml = (plantumlId: string, button: HTMLButtonElement) => {
            const body = document.querySelector(`[data-plantuml-id="${plantumlId}"]`) as HTMLElement;
            if (!body) return;

            const container = body.closest(".plantuml-container") as HTMLElement;
            if (!container) return;

            const code = decodeURIComponent(container.dataset.plantumlCode || "");

            const doCopy = () => {
                const copyIcon = button.querySelector(".copy-icon") as HTMLElement;
                const checkIcon = button.querySelector(".check-icon") as HTMLElement;

                if (copyIcon) copyIcon.style.display = "none";
                if (checkIcon) checkIcon.style.display = "inline-block";
                button.setAttribute("data-tooltip", backendStore.translate("Copied"));
                updateTooltipContent(button, backendStore.translate("Copied"));

                setTimeout(() => {
                    if (copyIcon) copyIcon.style.display = "inline-block";
                    if (checkIcon) checkIcon.style.display = "none";
                    button.setAttribute("data-tooltip", backendStore.translate("Copy"));
                    updateTooltipContent(button, backendStore.translate("Copy"));
                }, 2000);
            };

            if (!navigator.clipboard) {
                const textarea = document.createElement("textarea");
                textarea.value = code;
                textarea.style.position = "fixed";
                textarea.style.opacity = "0";
                document.body.appendChild(textarea);
                textarea.select();
                try {
                    if (document.execCommand("copy")) {
                        document.body.removeChild(textarea);
                        doCopy();
                    }
                } catch (err) {
                    console.error("[MarkdownRenderer] Copy plantuml failed:", err);
                    document.body.removeChild(textarea);
                }
                return;
            }

            navigator.clipboard.writeText(code).then(doCopy).catch((error) => {
                console.error("[MarkdownRenderer] Copy plantuml failed:", error);
            });
        };

        // Initialize sticky observer for code blocks
        const initCodeBlockStickyObserver = () => {
            const codeBlocks = document.querySelectorAll('.markdown-content .code-block-container');

            codeBlocks.forEach((container) => {
                const element = container as HTMLElement;

                // Skip if already initialized
                if (element.dataset.stickyInitialized === 'true') {
                    return;
                }

                element.dataset.stickyInitialized = 'true';

                // Create sentinel element for sticky detection
                const sentinel = document.createElement('div');
                sentinel.className = 'code-block-sentinel';
                sentinel.style.cssText = 'position: absolute; top: 0; left: 0; width: 1px; height: 1px; pointer-events: none; visibility: hidden;';
                element.style.position = 'relative';
                element.appendChild(sentinel);

                // Setup IntersectionObserver to detect sticky state
                const observer = new IntersectionObserver(([entry]) => {
                    if (!entry.isIntersecting) {
                        // Sentinel is out of viewport = header is stuck
                        element.classList.add('code-block-container--stuck');
                    } else {
                        // Sentinel is in viewport = header is not stuck
                        element.classList.remove('code-block-container--stuck');
                    }
                });

                observer.observe(sentinel);

                // Store observer for cleanup
                (element as any)._stickyObserver = observer;
            });
        };

        // Initialize table horizontal scroll with custom scrollbar
        const initTableScroll = () => {
            const tableWrappers = document.querySelectorAll('.markdown-content .table-wrapper');

            tableWrappers.forEach((wrapper) => {
                const element = wrapper as HTMLElement;

                // Skip if already initialized
                if (element.dataset.scrollInitialized === 'true') {
                    return;
                }

                element.dataset.scrollInitialized = 'true';

                // Find the scrollbar element (sibling)
                const tableBody = element.closest('.table-body');
                const scrollbar = tableBody?.querySelector('.table-scrollbar') as HTMLElement;
                const thumb = scrollbar?.querySelector('.table-scrollbar-thumb') as HTMLElement;

                if (!scrollbar || !thumb) {
                    return;
                }

                // Check if table needs horizontal scroll and update scrollbar state
                const checkScroll = () => {
                    const hasHorizontalScroll = element.scrollWidth > element.clientWidth;
                    if (hasHorizontalScroll) {
                        element.classList.add('has-scroll');
                        scrollbar.classList.add('visible');
                        updateTableScrollbar(element, thumb);
                    } else {
                        element.classList.remove('has-scroll');
                        scrollbar.classList.remove('visible');
                    }
                };
                checkScroll();
                // Check on resize
                const resizeObserver = new ResizeObserver(() => {
                    checkScroll();
                });
                resizeObserver.observe(element);

                // Handle scroll events
                let scrollTimeout: number | null = null;
                element.addEventListener('scroll', () => {
                    element.classList.add('scrolling');
                    updateTableScrollbar(element, thumb);

                    // Remove scrolling class after scroll ends
                    if (scrollTimeout) {
                        clearTimeout(scrollTimeout);
                    }
                    scrollTimeout = window.setTimeout(() => {
                        element.classList.remove('scrolling');
                    }, 1000);
                });

                // Handle mouse enter/leave for scrollbar visibility
                element.addEventListener('mouseenter', () => {
                    updateTableScrollbar(element, thumb);
                });

                // Handle mouse enter/leave on scrollbar itself
                scrollbar.addEventListener('mouseenter', () => {
                    scrollbar.classList.add('visible');
                });

                scrollbar.addEventListener('mouseleave', () => {
                    if (!thumb.classList.contains('dragging')) {
                        scrollbar.classList.remove('visible');
                    }
                });

                // Scrollbar thumb drag functionality
                let isDragging = false;
                let dragStartX = 0;
                let dragStartScrollLeft = 0;

                const handleThumbMouseDown = (e: MouseEvent) => {
                    e.preventDefault();
                    e.stopPropagation();
                    isDragging = true;
                    dragStartX = e.clientX;
                    dragStartScrollLeft = element.scrollLeft;
                    thumb.classList.add('dragging');

                    // Disable smooth scrolling during drag for immediate response
                    element.style.scrollBehavior = 'auto';

                    document.addEventListener('mousemove', handleThumbMouseMove);
                    document.addEventListener('mouseup', handleThumbMouseUp);
                };

                const handleThumbMouseMove = (e: MouseEvent) => {
                    if (!isDragging) return;

                    const scrollWidth = element.scrollWidth;
                    const clientWidth = element.clientWidth;
                    const trackWidth = clientWidth - 32; // Account for padding
                    const thumbWidth = Math.max(20, (clientWidth / scrollWidth) * trackWidth);
                    const maxScroll = scrollWidth - clientWidth;
                    const trackTravel = trackWidth - thumbWidth;

                    // Calculate the new thumb position based on current mouse position
                    const currentThumbPosition = ((dragStartScrollLeft / maxScroll) * trackTravel) + (e.clientX - dragStartX);
                    const clampedPosition = Math.max(0, Math.min(trackTravel, currentThumbPosition));

                    // Convert thumb position back to scroll position
                    element.scrollLeft = trackTravel > 0 ? (clampedPosition / trackTravel) * maxScroll : 0;
                };

                const handleThumbMouseUp = () => {
                    isDragging = false;
                    thumb.classList.remove('dragging');

                    // Re-enable smooth scrolling after drag
                    element.style.scrollBehavior = '';

                    document.removeEventListener('mousemove', handleThumbMouseMove);
                    document.removeEventListener('mouseup', handleThumbMouseUp);
                };

                thumb.addEventListener('mousedown', handleThumbMouseDown);

                // Click on track to jump
                scrollbar.addEventListener('click', (e: MouseEvent) => {
                    if (e.target === thumb) return; // Don't handle if clicking on thumb

                    const rect = scrollbar.getBoundingClientRect();
                    const clickX = e.clientX - rect.left;
                    const scrollWidth = element.scrollWidth;
                    const clientWidth = element.clientWidth;
                    const trackWidth = clientWidth - 32;
                    const thumbWidth = Math.max(20, (clientWidth / scrollWidth) * trackWidth);
                    const maxScroll = scrollWidth - clientWidth;
                    const trackTravel = trackWidth - thumbWidth;
                    const targetScroll = trackTravel > 0 ? (clickX / trackWidth) * maxScroll : 0;

                    element.scrollLeft = targetScroll;
                });

                // Handle wheel events for horizontal scroll
                element.addEventListener('wheel', (e: WheelEvent) => {
                    const hasHorizontalScroll = element.scrollWidth > element.clientWidth;
                    if (!hasHorizontalScroll) return;

                    // Check if we're at the start or end of horizontal scroll
                    const isAtStart = element.scrollLeft <= 0;
                    const isAtEnd = element.scrollLeft >= element.scrollWidth - element.clientWidth - 1;

                    // Handle shift + vertical scroll as horizontal scroll
                    if (e.shiftKey && Math.abs(e.deltaY) > Math.abs(e.deltaX)) {
                        e.preventDefault();
                        e.stopPropagation();
                        element.scrollLeft += e.deltaY;
                        updateTableScrollbar(element, thumb);
                        checkScrolledToEnd(element);
                    }
                    // If scrolling horizontally or at boundaries, prevent default
                    else if (Math.abs(e.deltaX) > Math.abs(e.deltaY) ||
                        (isAtStart && e.deltaX < 0) ||
                        (isAtEnd && e.deltaX > 0)) {
                        // Let the browser handle it naturally
                    }
                }, { passive: false });
            });
        };

        // Update custom scrollbar position
        const updateTableScrollbar = (wrapper: HTMLElement, thumb: HTMLElement) => {
            const scrollLeft = wrapper.scrollLeft;
            const scrollWidth = wrapper.scrollWidth;
            const clientWidth = wrapper.clientWidth;

            // Calculate scrollbar position
            const trackWidth = clientWidth - 32; // Account for padding (var(--spacing-md) * 2)
            const thumbWidth = Math.max(20, (clientWidth / scrollWidth) * trackWidth);
            const maxScroll = scrollWidth - clientWidth;
            const thumbPosition = maxScroll > 0 ? (scrollLeft / maxScroll) * (trackWidth - thumbWidth) : 0;

            // Update thumb width and position
            thumb.style.width = `${thumbWidth}px`;
            thumb.style.left = `${thumbPosition}px`;
        };

        // Expose copy and toggle functions to window for onclick attribute
        onMounted(async () => {
            (window as any).copyCode = copyCode;
            (window as any).toggleCodeBlock = toggleCodeBlock;
            (window as any).copyTable = copyTable;
            (window as any).copyMermaid = copyMermaid;
            (window as any).copyPlantUml = copyPlantUml;
            (window as any).toggleMermaidView = toggleMermaidView;
            (window as any).togglePlantUmlView = togglePlantUmlView;

            checkDarkMode();
            calculateUnitU();

            // Load MathJax
            await loadMathJax();

            // Initialize code block sticky observer, table scroll and tooltips after a short delay to ensure DOM is ready
            setTimeout(() => {
                initMermaidDiagrams();
                initMermaidScroll();
                initPlantUmlScroll();
                initCodeBlockStickyObserver();
                initTableScroll();
                initTooltips();
            }, 100);

            // Listen for dark mode changes
            const observer = new MutationObserver(() => {
                checkDarkMode();
            });

            observer.observe(document.documentElement, {
                attributes: true,
                attributeFilter: ["class"],
            });

            // Listen for system theme changes
            window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", checkDarkMode);

            // Observe DOM changes to initialize table scroll and code block sticky observer for new elements
            const contentObserver = new MutationObserver((mutations) => {
                let shouldInitTable = false;
                let shouldInitCodeBlock = false;
                let shouldInitTooltips = false;
                let shouldInitMermaid = false;
                mutations.forEach((mutation) => {
                    if (mutation.type === 'childList' && mutation.addedNodes.length > 0) {
                        mutation.addedNodes.forEach((node) => {
                            if (node instanceof HTMLElement) {
                                // Check if the added node or its children contain table wrappers
                                if (node.classList?.contains('table-wrapper') ||
                                    node.querySelector?.('.table-wrapper')) {
                                    shouldInitTable = true;
                                }
                                // Check if the added node or its children contain code blocks
                                if (node.classList?.contains('code-block-container') ||
                                    node.querySelector?.('.code-block-container')) {
                                    shouldInitCodeBlock = true;
                                }
                                // Check if the added node or its children contain copy buttons
                                if (node.classList?.contains('copy-button') ||
                                    node.querySelector?.('.copy-button')) {
                                    shouldInitTooltips = true;
                                }
                                // Check if the added node or its children contain mermaid containers
                                if (node.classList?.contains('mermaid-container') ||
                                    node.querySelector?.('.mermaid-container')) {
                                    shouldInitMermaid = true;
                                }
                            }
                        });
                    }
                });
                if (shouldInitMermaid) {
                    initMermaidDiagrams();
                    initMermaidScroll();
                }
                if (shouldInitTable) {
                    initTableScroll();
                }
                if (shouldInitCodeBlock) {
                    initCodeBlockStickyObserver();
                }
                if (shouldInitTooltips) {
                    initTooltips();
                }
            });

            // Start observing the document body for added nodes
            contentObserver.observe(document.body, {
                childList: true,
                subtree: true,
            });
        });

        return {
            renderedContent,
            renderMarkdown,
            isDark,
            markdownRef,
        };
    },
    render() {
        return <span class="markdown-content" ref="markdownRef" innerHTML={this.renderMarkdown} v-html={this.renderMarkdown} />;
    },
});
