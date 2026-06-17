(function() {
    'use strict';

    // ==================== 配置 ====================
    const CONFIG = {
        // 正文候选选择器（优先级从高到低）
        contentSelectors: [
            'article', 'main', '[role="main"]',
            '.post', '.entry', '.content', '.article',
            '.post-content', '.entry-content', '.article-content',
            '.news-content', '.article-body', '.main-content'
        ],

        // 噪音元素选择器（直接移除）
        noiseSelectors: [
            'nav', 'header', 'footer', 'aside',
            '.sidebar', '.sidebar-container', '.widget',
            '.ad', '.advertisement', '.ads', '.banner',
            '.sponsor', '.sponsored', '.promo', '.promotion',
            '.comments', '.comment', '.social-share',
            '.related-posts', '.recommendations', '.recommend',
            'script', 'style', 'noscript', 'iframe',
            '.cookie-notice', '.popup', '.modal',
            '[id*="ad-"]', '[id*="ad_"]', '[class*="ad-"]', '[class*="ad_"]'
        ],

        // 最小正文长度
        minContentLength: 100,

        // 调试模式
        debug: false
    };

    // 安全获取文本
    function getText(elem) {
        return elem ? (elem.innerText || elem.textContent || '').trim() : '';
    }

    // ==================== 移除所有图片 ====================

    function removeAllImages(root) {
        const images = Array.from(root.querySelectorAll('img'));
        for (const img of images) {
            if (img.parentNode) {
                img.remove();
            }
        }
    }

    // ==================== 移除所有链接（包括链接文本） ====================

    function removeAllLinks(root) {
        const links = Array.from(root.querySelectorAll('a'));
        for (const link of links) {
            if (link.parentNode) {
                link.remove();  // 直接删除整个链接元素，包括其文本内容
            }
        }
    }

    // ==================== 正文提取核心 ====================

    // 移除噪音元素
    function removeNoise(root) {
        // 移除固定/绝对定位元素
        const fixed = root.querySelectorAll('[style*="position: fixed"], [style*="position:fixed"], [style*="position: absolute"], [style*="position:absolute"]');
        fixed.forEach(el => el.remove());

        // 移除噪音选择器
        CONFIG.noiseSelectors.forEach(selector => {
            try {
                root.querySelectorAll(selector).forEach(el => el.remove());
            } catch(e) {}
        });
    }

    // 寻找最佳正文容器
    function findBestContent(root) {
        let candidates = [];

        // 优先使用语义选择器
        for (const selector of CONFIG.contentSelectors) {
            const elements = root.querySelectorAll(selector);
            if (elements.length > 0) {
                candidates = Array.from(elements);
                break;
            }
        }

        // 降级：使用所有 div/section
        if (candidates.length === 0) {
            candidates = Array.from(root.querySelectorAll('div, section'));
        }

        let bestElem = null;
        let bestScore = 0;

        for (const elem of candidates) {
            const text = getText(elem);
            const textLen = text.length;
            if (textLen < CONFIG.minContentLength) continue;

            // 段落加分
            const pCount = elem.querySelectorAll('p').length;
            const pScore = Math.min(pCount, 20) * 10;

            // 标题加分
            const hasHeading = elem.querySelector('h1, h2, h3') !== null;
            const headingScore = hasHeading ? 50 : 0;

            // 得分：文本长度 + 段落分 + 标题分
            let score = textLen + pScore + headingScore;

            if (score > bestScore) {
                bestScore = score;
                bestElem = elem;
            }
        }

        return { elem: bestElem, score: bestScore };
    }

    // 文本清洗
    function cleanText(text) {
        if (!text) return '';

        // 合并空白
        let cleaned = text.replace(/\s+/g, ' ');

        // 中文句末换行
        cleaned = cleaned.replace(/([。！？])\s+/g, '$1\n');

        // 压缩连续换行
        cleaned = cleaned.replace(/\n{3,}/g, '\n\n');

        // 删除无效行
        const lines = cleaned.split('\n');
        const valid = lines.filter(line => {
            const trimmed = line.trim();
            return trimmed.length > 2 && /[\u4e00-\u9fa5a-zA-Z0-9]/.test(trimmed);
        });

        cleaned = valid.join('\n');
        cleaned = cleaned.replace(/[ \t]+/g, ' ');
        cleaned = cleaned.trim();

        return cleaned;
    }

    // ==================== 主流程 ====================

    try {
        // 获取标题
        const title = document.title || '';

        // 克隆 DOM
        const clone = document.body.cloneNode(true);

        // 移除所有图片
        removeAllImages(clone);

        // 移除所有链接（包括链接文本）
        removeAllLinks(clone);

        // 移除噪音
        removeNoise(clone);

        // 寻找正文容器
        const { elem: contentElem, score } = findBestContent(clone);

        let contentText = '';
        if (contentElem && score > CONFIG.minContentLength) {
            contentText = getText(contentElem);
        } else {
            // 降级：提取所有长段落
            const paragraphs = clone.querySelectorAll('p');
            const texts = [];
            for (const p of paragraphs) {
                const pText = getText(p);
                if (pText.length > 50) {
                    texts.push(pText);
                }
            }
            contentText = texts.join('\n\n');
        }

        // 清洗文本
        contentText = cleanText(contentText);

        // 获取 meta 描述
        const metaTag = document.querySelector('meta[name="description"]');
        const metaDescription = metaTag ? (metaTag.getAttribute('content') || '') : '';

        return JSON.stringify({
            success: true,
            title: title,
            metaDescription: metaDescription,
            content: contentText,
            contentLength: contentText.length,
            confidence: Math.min(100, Math.floor(score / 20))
        });

    } catch(e) {
        console.error('[Extractor] 错误:', e);
        return JSON.stringify({
            success: false,
            error: e.message,
            stack: CONFIG.debug ? e.stack : undefined
        });
    }

})();
