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
        const backendStore = useBackendStore();

        // MathJax promise to ensure it's loaded
        let mathJaxPromise: Promise<void> | null = null;

        // Load MathJax from CDN
        const loadMathJax = (): Promise<void> => {
            if ((window as any).MathJax && (window as any).MathJax.tex2svg) {
                return Promise.resolve();
            }

            if (mathJaxPromise) {
                return mathJaxPromise;
            }

            mathJaxPromise = new Promise<void>((resolve, reject) => {
                // Configure MathJax before loading
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

                // Load MathJax script
                const script = document.createElement('script');
                script.src = 'https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-svg.js';
                script.async = true;
                script.onload = () => {
                    // Give MathJax time to initialize
                    setTimeout(resolve, 100);
                };
                script.onerror = reject;
                document.head.appendChild(script);
            });

            return mathJaxPromise;
        };

        // Render math with MathJax
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

        // Create a marked extension for MathJax rendering
        const mathExtension = {
            name: "math",
            level: "inline",
            start(src: string) {
                return src.indexOf("$");
            },
            tokenizer(src: string) {
                // Match inline math: $...$
                const inlineMatch = src.match(/^\$([^$\n]+?)\$/);
                if (inlineMatch) {
                    return {
                        type: "math",
                        raw: inlineMatch[0],
                        text: inlineMatch[1],
                        displayMode: false,
                    };
                }

                // Match block math: $$...$$
                const blockMatch = src.match(/^\$\$([\s\S]+?)\$\$/);
                if (blockMatch && blockMatch[1]) {
                    return {
                        type: "math",
                        raw: blockMatch[0],
                        text: blockMatch[1]!.trim(),
                        displayMode: true,
                    };
                }

                return undefined;
            },
            renderer(token: any) {
                try {
                    const renderedMath = renderMath(token.text, token.displayMode);

                    // Wrap with CSS classes (styles are defined in CSS file)
                    if (token.displayMode) {
                        // Block math - left aligned to prevent jitter during streaming
                        return `<div class="mathjax-math-container">${renderedMath}</div>`;
                    } else {
                        // Inline math
                        return `<span class="mathjax-math-inline">${renderedMath}</span>`;
                    }
                } catch (error) {
                    console.error("[MarkdownRenderer] MathJax render error:", error);
                    return token.raw;
                }
            },
        };

        // Configure marked with math extension
        marked.use({
            extensions: [mathExtension],
        });

        marked.setOptions({
            breaks: true, // Enable GitHub-flavored line breaks
            gfm: true, // Enable GitHub Flavored Markdown
        });

        // Check if dark mode is active
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

                // Process images to add context menu for copy
                const images = tempDiv.querySelectorAll("img");
                images.forEach((img, index) => {
                    const imageId = `image-${Date.now()}-${index}`;
                    img.setAttribute("data-image-id", imageId);
                    img.style.cursor = "pointer";
                    img.setAttribute("title", "Right-click to copy image");
                });

                finalHtml = tempDiv.innerHTML;

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

        // Image copy function
        const copyImage = async (imageId: string) => {
            const img = document.querySelector(`[data-image-id="${imageId}"]`) as HTMLImageElement;
            if (!img) return;

            // Method 1: Try to copy using canvas
            try {
                const canvas = document.createElement("canvas");
                canvas.width = img.naturalWidth || img.width;
                canvas.height = img.naturalHeight || img.height;
                const ctx = canvas.getContext("2d");

                if (ctx) {
                    ctx.drawImage(img, 0, 0);

                    // Convert to blob
                    canvas.toBlob(async (blob) => {
                        if (blob) {
                            try {
                                await navigator.clipboard.write([
                                    new ClipboardItem({
                                        [blob.type]: blob,
                                    }),
                                ]);
                                console.log("Image copied to clipboard (Canvas method)");
                            } catch (error) {
                                console.error("Canvas copy failed:", error);
                                tryImageURLFallback(img);
                            }
                        } else {
                            console.error("Canvas blob generation failed");
                            tryImageURLFallback(img);
                        }
                    }, "image/png");
                    return;
                }
            } catch (error) {
                console.error("Canvas execution failed:", error);
            }

            // Method 2: Try fetch
            tryFetchImage(img);
        };

        // Try fetch image blob
        const tryFetchImage = async (img: HTMLImageElement) => {
            try {
                // Check if src is data URL
                if (img.src.startsWith("data:")) {
                    tryDataURLCopy(img.src);
                    return;
                }

                const response = await fetch(img.src);
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                const blob = await response.blob();

                // Copy to clipboard
                await navigator.clipboard.write([
                    new ClipboardItem({
                        [blob.type]: blob,
                    }),
                ]);

                console.log("Image copied to clipboard (Fetch method)");
            } catch (error) {
                console.error("Fetch copy failed:", error);
                tryImageURLFallback(img);
            }
        };

        // Try copy data URL directly
        const tryDataURLCopy = async (dataURL: string) => {
            try {
                // Extract the base64 part
                const base64Data = dataURL.split(",")[1];
                if (!base64Data) {
                    throw new Error("Invalid data URL");
                }

                // Convert to blob
                const byteCharacters = atob(base64Data);
                const byteNumbers = new Array(byteCharacters.length);
                for (let i = 0; i < byteCharacters.length; i++) {
                    byteNumbers[i] = byteCharacters.charCodeAt(i);
                }
                const byteArray = new Uint8Array(byteNumbers);
                const blob = new Blob([byteArray], { type: "image/png" });

                await navigator.clipboard.write([
                    new ClipboardItem({
                        "image/png": blob,
                    }),
                ]);

                console.log("Image copied to clipboard (DataURL method)");
            } catch (error) {
                console.error("DataURL copy failed:", error);
                tryTextFallback(dataURL);
            }
        };

        // Fallback: copy image URL
        const tryImageURLFallback = (img: HTMLImageElement) => {
            try {
                navigator.clipboard
                    .writeText(img.src)
                    .then(() => {
                        console.log("Image URL copied to clipboard");
                    })
                    .catch((error) => {
                        console.error("Copy image URL failed:", error);
                        console.log("Image URL:", img.src);
                        // Try using execCommand as last resort
                        tryExecCommandCopy(img.src);
                    });
            } catch (error) {
                console.error("clipboard.writeText failed:", error);
                tryExecCommandCopy(img.src);
            }
        };

        // Try copy text as last resort
        const tryTextFallback = (text: string) => {
            try {
                navigator.clipboard
                    .writeText(text)
                    .then(() => {
                        console.log("Text copied to clipboard");
                    })
                    .catch((error) => {
                        console.error("Copy text failed:", error);
                    });
            } catch (error) {
                console.error("tryTextFallback failed:", error);
            }
        };

        // Last resort: execCommand
        const tryExecCommandCopy = (text: string) => {
            const textarea = document.createElement("textarea");
            textarea.value = text;
            textarea.style.position = "fixed";
            textarea.style.opacity = "0";
            document.body.appendChild(textarea);
            textarea.select();

            try {
                const successful = document.execCommand("copy");
                document.body.removeChild(textarea);
                if (successful) {
                    console.log("Successfully copied using execCommand");
                } else {
                    console.log("execCommand copy failed");
                    console.log("Please copy manually:", text);
                }
            } catch (err) {
                console.error("execCommand execution failed:", err);
                document.body.removeChild(textarea);
                console.log("Please copy manually:", text);
            }
        };

        // Handle context menu on images
        const handleImageContextMenu = (event: MouseEvent) => {
            const target = event.target as HTMLElement;
            if (target.tagName === "IMG") {
                const img = target as HTMLImageElement;
                const imageId = img.getAttribute("data-image-id");

                if (imageId) {
                    // Prevent default context menu
                    event.preventDefault();

                    // Create custom context menu
                    const menu = document.createElement("div");
                    menu.className = "markdown-image-context-menu";
                    menu.innerHTML = `
                        <div class="menu-item" data-action="copy" data-image-id="${imageId}">
                            <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16">
                                <path fill-opacity="1" fill="currentColor"  transform="translate(1 1)" d="M12 5C13.104569 5 14 5.8954306 14 7L14 12C14 13.104569 13.104569 14 12 14L7 14C5.8954306 14 5 13.104569 5 12L5 7C5 5.8954306 5.8954306 5 7 5L12 5ZM7 0C8.1045694 0 9 0.89543051 9 2L9 4L8 4L8 2C8 1.4871641 7.6139598 1.0644928 7.116621 1.0067277L7 1L2 1C1.4871641 1 1.0644928 1.3860402 1.0067277 1.8833789L1 2L1 7C1 7.512836 1.3860402 7.9355073 1.8833789 7.9932723L2 8L4 8L4 9L2 9C0.89543051 9 0 8.1045694 0 7L0 2C0 0.89543051 0.89543051 0 2 0L7 0Z"/>
                            </svg>
                            <span>Copy Image</span>
                        </div>
                    `;

                    // Position menu
                    menu.style.position = "fixed";
                    menu.style.left = `${event.clientX}px`;
                    menu.style.top = `${event.clientY}px`;
                    menu.style.zIndex = "10000";
                    menu.style.backgroundColor = "white";
                    menu.style.border = "1px solid #ddd";
                    menu.style.borderRadius = "8px";
                    menu.style.boxShadow = "0 2px 10px rgba(0,0,0,0.1)";
                    menu.style.minWidth = "150px";
                    menu.style.padding = "4px 0";

                    // Add styles to menu items
                    const style = document.createElement("style");
                    style.textContent = `
                        .markdown-image-context-menu .menu-item {
                            display: flex;
                            align-items: center;
                            gap: 8px;
                            padding: 8px 16px;
                            cursor: pointer;
                            font-size: 14px;
                            color: #333;
                        }
                        .markdown-image-context-menu .menu-item:hover {
                            background-color: #f5f5f5;
                        }
                        .markdown-image-context-menu .menu-item svg {
                            width: 16px;
                            height: 16px;
                        }
                    `;
                    document.head.appendChild(style);

                    // Add click handler
                    menu.addEventListener("click", (e) => {
                        const target = e.target as HTMLElement;
                        const menuItem = target.closest(".menu-item") as HTMLElement;
                        if (menuItem) {
                            const action = menuItem.getAttribute("data-action");
                            const id = menuItem.getAttribute("data-image-id");

                            if (action === "copy" && id) {
                                copyImage(id);
                            }
                        }

                        // Remove menu
                        document.body.removeChild(menu);
                    });

                    // Remove menu on click outside
                    const removeMenu = () => {
                        if (document.body.contains(menu)) {
                            document.body.removeChild(menu);
                        }
                        document.removeEventListener("click", removeMenu);
                    };

                    setTimeout(() => {
                        document.addEventListener("click", removeMenu);
                    }, 0);

                    document.body.appendChild(menu);
                }
            }
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
                    initCodeBlockStickyObserver();
                    initTableScroll();
                    initTooltips();
                }, 50);
            });
        });

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
            (window as any).copyImage = copyImage;

            checkDarkMode();
            calculateUnitU();

            // Load MathJax
            await loadMathJax();

            // Initialize code block sticky observer, table scroll and tooltips after a short delay to ensure DOM is ready
            setTimeout(() => {
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

            // Add context menu listener for images
            document.addEventListener("contextmenu", handleImageContextMenu);

            // Observe DOM changes to initialize table scroll and code block sticky observer for new elements
            const contentObserver = new MutationObserver((mutations) => {
                let shouldInitTable = false;
                let shouldInitCodeBlock = false;
                let shouldInitTooltips = false;
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
                            }
                        });
                    }
                });
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
        };
    },
    render() {
        return <span class="markdown-content" innerHTML={this.renderMarkdown} v-html={this.renderMarkdown} />;
    },
});
