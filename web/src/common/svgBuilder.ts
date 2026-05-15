import { readFileSync, readdirSync } from 'fs';

let idPerfix = '';
const svgTitle = /<svg([^>+].*?)>/;
const clearHeightWidth = /(width|height)="([^>+].*?)"/g;
const hasViewBox = /(viewBox="[^>+].*?")/g;
const clearReturn = /(\r)|(\n)/g;

// 转义正则表达式特殊字符
function escapeRegExp(str: string): string {
    return str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function prefixIds(svgContent: string, prefix: string): string {
    // 收集所有 ID 定义（排除已经添加了前缀的 symbol id）
    const ids: string[] = [];
    const idDefinition = /id="([^"]+)"/g;
    let match;
    while ((match = idDefinition.exec(svgContent)) !== null) {
        // 排除我们刚刚添加的 symbol id (格式为 icon-filename)
        if (!match[1].startsWith('icon-')) {
            ids.push(match[1]);
        }
    }
    
    // 为每个 ID 添加前缀
    let result = svgContent;
    for (const id of ids) {
        const escapedId = escapeRegExp(id);
        // 替换 id="xxx" 为 id="prefix-xxx"
        result = result.replace(new RegExp(`id="${escapedId}"`, 'g'), `id="${prefix}-${id}"`);
        // 替换 url(#xxx) 为 url(#prefix-xxx)
        result = result.replace(new RegExp(`url\\(#${escapedId}\\)`, 'g'), `url(#${prefix}-${id})`);
        // 替换 xlink:href="#xxx" 为 xlink:href="#prefix-xxx"
        result = result.replace(new RegExp(`xlink:href="#${escapedId}"`, 'g'), `xlink:href="#${prefix}-${id}"`);
    }
    
    return result;
}

function findSvgFile(dir: string): string[] {
    const svgRes: string[] = [];
    const dirents = readdirSync(dir, {
        withFileTypes: true,
    });
    for (const dirent of dirents) {
        if (dirent.isDirectory()) {
            svgRes.push(...findSvgFile(dir + dirent.name + '/'));
        } else {
            const fileName = dirent.name.replace('.svg', '');
            const svg = readFileSync(dir + dirent.name)
                .toString()
                .replace(clearReturn, '')
                .replace(svgTitle, ($1, $2) => {
                    let width = 0;
                    let height = 0;
                    let content = $2.replace(clearHeightWidth, (s1, s2, s3) => {
                        if (s2 === 'width') {
                            width = s3;
                        } else if (s2 === 'height') {
                            height = s3;
                        }
                        return '';
                    });
                    if (!hasViewBox.test($2)) {
                        content += `viewBox="0 0 ${width} ${height}"`;
                    }
                    return `<symbol id="${idPerfix}-${fileName}" ${content}>`;
                })
                .replace('</svg>', '</symbol>');
            
            // 为内部 ID 添加文件名前缀，避免跨 SVG 的 ID 冲突
            const prefixedSvg = prefixIds(svg, fileName);
            svgRes.push(prefixedSvg);
        }
    }
    return svgRes;
}

export const svgBuilder = (path: string, perfix = 'icon') => {
    if (path === '') return;
    idPerfix = perfix;
    const res = findSvgFile(path);
    return {
        name: 'svg-transform',
        transformIndexHtml(html: string) {
            return html.replace(
                '<body>',
                `
          <body>
            <svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" style="position: absolute; width: 0; height: 0">
              ${res.join('')}
            </svg>
        `
            );
        },
    };
};
