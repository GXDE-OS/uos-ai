<template>
    <div class="outline" v-show="show" ref="outlineRef" :class="{ disabled: disabled }">
        <div class="outline-title" :class="{'focused': isInputFocused}">
            <OutlineInputArea style="margin-left: 10px;"
                    v-model:value="Title" 
                    :placeholder="titlePlaceholder" 
                    :textColor="'var(--uosai-color-outline-paragraph-text)'"
                    :fontSize="'1.15rem'"
                    :fontWeight="'500'"
                    @edit-complete="handleOutlineTitleEditComplete"
                    @isInputFocus="handleIsInputFocus"
                />
        </div>
        <!-- 段落 -->
        <div 
            v-for="(item, index) in Paragraphs" 
            :key="index"
            class="outline-paragraph-wrapper"
            :class="{ 
                'drag-over': paragraphDragOverIndex === index,
                'drag-over-before': paragraphDragOverIndex === index && paragraphDragDirection === 'up',
                'drag-over-after': paragraphDragOverIndex === index && paragraphDragDirection === 'down',
                'dragging': paragraphDragState.isDragging && paragraphDragState.draggedIndex === index
            }"
        >
            <OutlineParagraph 
                :title="item.title" 
                :content="item.content" 
                :index="index"
                @update:content="(newContent) => updateParagraphContent(index, newContent)"
                @update:title="(newTitle) => updateParagraphTitle(index, newTitle)"
                @update:description="(newDescription) => updateParagraphDescription(index, newDescription)"
                @drag-start="handleParagraphDragStart"
                @drag-move="handleParagraphDragMove"
                @drag-end="handleParagraphDragEnd"
                @delete-item="(itemIndex) => handleDeleteItem(index, itemIndex)"
                @delete-paragraph="handleDeleteParagraph"
            />
        </div>
        <!-- 添加章节 -->
        <div class="outline-add" @click="addParagraph" :class="{ disabled: disabled }">
            <SvgIcon icon="add-outline" />
            <div class="outline-add-text">
                {{  store.loadTranslations['Add Chapter']}}
            </div>
        </div>
    </div>
</template>

<script setup>
import { ref, watch, onMounted, computed, nextTick, defineProps, onUnmounted } from 'vue'
import SvgIcon from '@/components/svgIcon/svgIcon.vue'
import OutlineParagraph from './OutlineParagraph.vue';
import OutlineInputArea from './OutlineInputArea.vue';

import { Qrequest } from "@/utils";
import { useGlobalStore } from "@/store/global";
const { chatQWeb } = useGlobalStore();
const store = useGlobalStore()

const props = defineProps({
    show: {
        type: Boolean,
        default: false
    },
    Title: {
        type: String,
        default: ''
    },
    Paragraphs: {
        type: Array,
        default: () => []
    },
    disabled: {
        type: Boolean,
        default: false
    },
});

const emit = defineEmits(['updateOutline'])
const titlePlaceholder = computed(() => {
    return store.loadTranslations['Please input the title'] || 'Please input the title';
})
const Title = computed(() => {
    return props.Title
})

// 段落数组
const Paragraphs = ref([])

watch(
    () => props.Paragraphs,
    (newPropsParagraphs) => {
        // 执行原计算属性的过滤逻辑
        Paragraphs.value = newPropsParagraphs.map(paragraph => {
            if (!paragraph) return paragraph
            
            // 如果content存在且是数组，过滤掉没有title字段的对象
            if (Array.isArray(paragraph.content)) {
                return {
                    ...paragraph,
                    content: paragraph.content.filter(item => item && item.hasOwnProperty('title'))
                }
            }
            return paragraph
        })
    },
    { immediate: true, deep: true }
);

// -------------------------
// 自动标号（根据当前顺序重排标题前缀）
// 说明：AI 生成的大纲标题常带有“1.”、“一、”、“（1）”、“1.1”等前缀；
//      当用户拖拽重排/删除/新增后，这些前缀不会自动变化，因此这里在本地变更后做一次重算。
// -------------------------
const CHINESE_NUMERAL_TOKEN_RE = /^[零一二三四五六七八九十百千]+$/

const isChineseNumeralToken = (token) => CHINESE_NUMERAL_TOKEN_RE.test(token)

const escapeRegExp = (str) => str.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')

const parseChineseNumeralToken = (token) => {
    if (typeof token !== 'string' || !token) return null
    const digits = {
        零: 0,
        一: 1,
        二: 2,
        三: 3,
        四: 4,
        五: 5,
        六: 6,
        七: 7,
        八: 8,
        九: 9
    }
    const units = { 十: 10, 百: 100, 千: 1000, 万: 10000 }

    // 含单位：按权重解析（十二/一百零二/一千二百三十四）
    if (/[十百千万]/.test(token)) {
        let total = 0
        let section = 0
        let number = 0
        for (const ch of token) {
            if (digits[ch] !== undefined) {
                number = digits[ch]
                continue
            }
            const unit = units[ch]
            if (!unit) return null
            if (unit === 10000) {
                section = (section + number) * unit
                total += section
                section = 0
                number = 0
                continue
            }
            section += (number || 1) * unit
            number = 0
        }
        total += section + number
        return Number.isFinite(total) ? total : null
    }

    // 不含单位：按数字逐位（常见于年份“二零二五”）
    let s = ''
    for (const ch of token) {
        if (digits[ch] === undefined) return null
        s += String(digits[ch])
    }
    const n = Number(s)
    return Number.isFinite(n) ? n : null
}

const parseOrdinalToken = (token) => {
    if (typeof token !== 'string' || !token) return null
    if (/^\d+$/.test(token)) {
        const n = Number(token)
        return Number.isFinite(n) ? n : null
    }
    if (isChineseNumeralToken(token)) return parseChineseNumeralToken(token)
    return null
}

const isPlausibleIndexToken = (token, max) => {
    const n = parseOrdinalToken(token)
    if (!Number.isFinite(n)) return false
    if (!Number.isFinite(max) || max <= 0) return false
    return n >= 1 && n <= max
}

const isLikelyIndexToken = (token, max) => {
    const n = parseOrdinalToken(token)
    if (!Number.isFinite(n) || n < 1) return false
    if (!Number.isFinite(max) || max <= 0) return n <= 999
    const upper = Math.min(999, max + 200)
    return n <= upper
}

const toChineseNumeral = (num) => {
    const n = Number(num)
    if (!Number.isFinite(n) || n <= 0) return String(num)
    const digits = ['零', '一', '二', '三', '四', '五', '六', '七', '八', '九']
    if (n < 10) return digits[n]
    if (n < 20) return n === 10 ? '十' : `十${digits[n % 10]}`
    const units = ['', '十', '百', '千', '万']
    const chars = String(n).split('').map((d) => Number(d))
    let result = ''
    let zeroPending = false
    for (let i = 0; i < chars.length; i++) {
        const digit = chars[i]
        const pos = chars.length - i - 1
        const unit = units[pos] || ''
        if (digit === 0) {
            if (result !== '') zeroPending = true
            continue
        }
        if (zeroPending) {
            result += '零'
            zeroPending = false
        }
        result += digits[digit] + unit
    }
    return result
}

const formatOrdinal = (num, numeralType) => {
    if (numeralType === 'chinese') return toChineseNumeral(num)
    return String(num)
}

const NUMBERING_SEPARATOR_RE = /^[、.．:：)）－—（()-]/

const isIncompleteNumberingPrefix = (text, max) => {
    if (typeof text !== 'string') return false
    const s = text.trimStart()
    const m = s.match(/^(\d+|[零一二三四五六七八九十百千]+)([\s\S]*)$/)
    if (!m) return false
    const token = m[1]
    if (!isLikelyIndexToken(token, max)) return false

    const after = m[2] || ''
    if (!after) return true
    if (/^\s+/.test(after)) return false

    const afterTrim = after.trimStart()
    if (!afterTrim) return true
    return !NUMBERING_SEPARATOR_RE.test(afterTrim)
}

const isIncompleteCompoundNumberingPrefix = (text, paragraphCount, itemCount) => {
    if (typeof text !== 'string') return false
    const s = text.trimStart()
    const m = s.match(/^(\d+)\s*[\\.．]\s*(\d+)([\s\S]*)$/)
    if (!m) return false

    const a = Number(m[1])
    const b = Number(m[2])
    const looksLikeNumbering =
        Number.isFinite(a) &&
        Number.isFinite(b) &&
        a >= 1 &&
        b >= 1 &&
        (Number.isFinite(paragraphCount) && paragraphCount > 0 ? a <= paragraphCount : true) &&
        (Number.isFinite(itemCount) && itemCount > 0 ? b <= itemCount : true)

    if (!looksLikeNumbering) return false

    const after = m[3] || ''
    if (!after) return true
    if (/^\s+/.test(after)) return false

    const afterTrim = after.trimStart()
    if (!afterTrim) return true
    return !/^[、.．:：)）－—-]/.test(afterTrim)
}

const stripAnyNumberingPrefix = (text, max) => {
    if (typeof text !== 'string') return text
    const s = text.trimStart()

    // （一） / (1)
    const mParen = s.match(/^[（(]\s*(\d+|[零一二三四五六七八九十百千]+)\s*[）)]\s*([\s\S]*)$/)
    if (mParen && isLikelyIndexToken(mParen[1], max)) {
        return (mParen[2] || '').trimStart()
    }

    // 1. / 1、 / 1) / 一、 / 1： / 1-（点号/冒号/短横后不能直接跟数字，避免误伤 3.14 / 12:34 / 2025-12-23）
    const mDelim = s.match(
        /^(\d+|[零一二三四五六七八九十百千]+)\s*([、)）]|[\\.．](?!\d)|[:：](?!\d)|[-－—](?!\d))\s*([\s\S]*)$/
    )
    if (mDelim && isLikelyIndexToken(mDelim[1], max)) {
        return (mDelim[3] || '').trimStart()
    }

    // 仅空格分隔：1 标题
    const mSpace = s.match(/^(\d+|[零一二三四五六七八九十百千]+)\s+([\s\S]*)$/)
    if (mSpace && isLikelyIndexToken(mSpace[1], max)) {
        return (mSpace[2] || '').trimStart()
    }

    return s
}

const detectLevel1Style = (paragraphs) => {
    if (!Array.isArray(paragraphs) || paragraphs.length === 0) return null

    for (const p of paragraphs) {
        const t = typeof p?.title === 'string' ? p.title : ''
        if (!t) continue

        // （一）/ (1)
        const mParen = t.match(/^\s*([（(])\s*([0-9]+|[零一二三四五六七八九十百千]+)\s*([）)])(\s*)/)
        if (mParen) {
            if (!isLikelyIndexToken(mParen[2], paragraphs.length)) continue
            return {
                kind: 'paren',
                numeral: isChineseNumeralToken(mParen[2]) ? 'chinese' : 'arabic',
                open: mParen[1],
                close: mParen[3],
                space: mParen[4]?.length ? ' ' : ''
            }
        }

        // 1. / 1、 / 1) / 一、 / 1： / 1-（点号/冒号/短横后不能直接跟数字，避免误伤 3.14 / 12:34 / 2025-12-23）
        const mDelim = t.match(/^\s*([0-9]+|[零一二三四五六七八九十百千]+)\s*([、)）]|[\\.．](?!\d)|[:：](?!\d)|[-－—](?!\d))(\s*)/)
        if (mDelim) {
            if (!isLikelyIndexToken(mDelim[1], paragraphs.length)) continue
            return {
                kind: 'delim',
                numeral: isChineseNumeralToken(mDelim[1]) ? 'chinese' : 'arabic',
                delim: mDelim[2],
                space: mDelim[3]?.length ? ' ' : ''
            }
        }
    }

    // 仅空格分隔：1 标题（需要数值落在段落数量范围内，降低误判）
    const max = paragraphs.length
    let hits = 0
    for (const p of paragraphs) {
        const t = typeof p?.title === 'string' ? p.title : ''
        const mSpace = t.match(/^\s*(\d+)\s+/)
        if (!mSpace) continue
        const v = Number(mSpace[1])
        if (Number.isFinite(v) && v >= 1 && v <= max) hits++
    }
    if (hits >= Math.min(2, max)) {
        return { kind: 'space', numeral: 'arabic', space: ' ' }
    }

    return null
}

const detectLevel2Style = (items) => {
    if (!Array.isArray(items) || items.length === 0) return null

    for (const it of items) {
        const t = typeof it?.title === 'string' ? it.title : ''
        if (!t) continue

        // 1.1 / 1．1（要求后面有分隔符或空白，避免误判；允许“1.1：”这类分隔符）
        const mCompound = t.match(/^\s*(\d+)([\\.．])(\d+)([、.．:：)）－—-])?(\s*)/)
        if (mCompound) {
            const hasSeparator = !!mCompound[4] || (mCompound[5]?.length ?? 0) > 0
            const second = Number(mCompound[3])
            const looksLikeIndex = Number.isFinite(second) && second >= 1 && second <= items.length
            if (hasSeparator && looksLikeIndex) {
                return {
                    kind: 'compound',
                    between: mCompound[2],
                    after: mCompound[4] || '',
                    space: mCompound[5]?.length ? ' ' : ''
                }
            }
        }

        // （一）/ (1)
        const mParen = t.match(/^\s*([（(])\s*([0-9]+|[零一二三四五六七八九十百千]+)\s*([）)])(\s*)/)
        if (mParen) {
            if (!isLikelyIndexToken(mParen[2], items.length)) continue
            return {
                kind: 'paren',
                numeral: isChineseNumeralToken(mParen[2]) ? 'chinese' : 'arabic',
                open: mParen[1],
                close: mParen[3],
                space: mParen[4]?.length ? ' ' : ''
            }
        }

        // 1. / 1、 / 1) / 一、 / 1： / 1-（点号/冒号/短横后不能直接跟数字，避免误判）
        const mDelim = t.match(/^\s*([0-9]+|[零一二三四五六七八九十百千]+)\s*([、)）]|[\\.．](?!\d)|[:：](?!\d)|[-－—](?!\d))(\s*)/)
        if (mDelim) {
            if (!isLikelyIndexToken(mDelim[1], items.length)) continue
            return {
                kind: 'delim',
                numeral: isChineseNumeralToken(mDelim[1]) ? 'chinese' : 'arabic',
                delim: mDelim[2],
                space: mDelim[3]?.length ? ' ' : ''
            }
        }
    }

    // 仅空格分隔：1 标题（需要数值落在条目数量范围内，降低误判）
    const max = items.length
    let hits = 0
    for (const it of items) {
        const t = typeof it?.title === 'string' ? it.title : ''
        const mSpace = t.match(/^\s*(\d+)\s+/)
        if (!mSpace) continue
        const v = Number(mSpace[1])
        if (Number.isFinite(v) && v >= 1 && v <= max) hits++
    }
    if (hits >= Math.min(2, max)) {
        return { kind: 'space', numeral: 'arabic', space: ' ' }
    }

    return null
}

const applyLevel1Numbering = (title, index, style, paragraphCount) => {
    if (typeof title !== 'string' || !title.trim()) return title
    if (!style) return title

    const expectedNum = formatOrdinal(index + 1, style.numeral)
    const trimmedTitle = title.trimStart()
    let rest = title
    let stripped = false

    if (style.kind === 'paren') {
        const re = new RegExp(
            `^\\s*${escapeRegExp(style.open)}\\s*(\\d+|[零一二三四五六七八九十百千]+)\\s*${escapeRegExp(style.close)}\\s*([\\s\\S]*)$`
        )
        const m = title.match(re)
        if (m && isLikelyIndexToken(m[1], paragraphCount)) {
            rest = m[2]
            stripped = true
        } else {
            rest = stripAnyNumberingPrefix(title, paragraphCount)
            stripped = rest !== trimmedTitle
        }
        rest = rest.trimStart()
        if (!rest) return title
        if (!stripped && isIncompleteNumberingPrefix(title, paragraphCount)) return title
        return `${style.open}${expectedNum}${style.close}${style.space}${rest}`
    }

    if (style.kind === 'delim') {
        const re = new RegExp(
            `^\\s*(\\d+|[零一二三四五六七八九十百千]+)\\s*${escapeRegExp(style.delim)}\\s*([\\s\\S]*)$`
        )
        const m = title.match(re)
        if (m && isLikelyIndexToken(m[1], paragraphCount)) {
            rest = m[2]
            stripped = true
        } else {
            rest = stripAnyNumberingPrefix(title, paragraphCount)
            stripped = rest !== trimmedTitle
        }
        rest = rest.trimStart()
        if (!rest) return title
        if (!stripped && isIncompleteNumberingPrefix(title, paragraphCount)) return title
        return `${expectedNum}${style.delim}${style.space}${rest}`
    }

    if (style.kind === 'space') {
        const re = /^\s*(\d+|[零一二三四五六七八九十百千]+)\s+([\s\S]*)$/
        const m = title.match(re)
        if (m && isLikelyIndexToken(m[1], paragraphCount)) {
            rest = m[2]
            stripped = true
        } else {
            rest = stripAnyNumberingPrefix(title, paragraphCount)
            stripped = rest !== trimmedTitle
        }
        rest = rest.trimStart()
        if (!rest) return title
        if (!stripped && isIncompleteNumberingPrefix(title, paragraphCount)) return title
        return `${expectedNum}${style.space}${rest}`
    }

    return title
}

const applyLevel2Numbering = (title, paragraphIndex, itemIndex, style, paragraphCount, itemCount) => {
    if (typeof title !== 'string' || !title.trim()) return title
    if (!style) return title

    // 1.1 风格：通常期望与段落号联动
    if (style.kind === 'compound') {
        const chapterNum = String(paragraphIndex + 1)
        const itemNum = String(itemIndex + 1)
        const totalParagraphs = Number.isFinite(paragraphCount) ? paragraphCount : 0
        const totalItems = Number.isFinite(itemCount) ? itemCount : 0

        // 先尝试解析出类似 "x.y " / "x.y、" / "x.y." 的前缀；如果 y 超出条目数量，视为正文数字（例如 3.14）
        const m = title.match(/^\s*(\d+)\s*[\\.．]\s*(\d+)(?:\s*[、.．:：)）－—-]\s*|\s+)([\s\S]*)$/)
        const trimmedTitle = title.trimStart()
        let rest = trimmedTitle
        let stripped = false
        if (m) {
            const a = Number(m[1])
            const b = Number(m[2])
            const looksLikeNumbering =
                Number.isFinite(a) &&
                Number.isFinite(b) &&
                a >= 1 &&
                b >= 1 &&
                (totalParagraphs ? a <= totalParagraphs : true) &&
                (totalItems ? b <= totalItems : true)
            rest = looksLikeNumbering ? m[3] : trimmedTitle
            stripped = looksLikeNumbering
        } else {
            // 匹配失败则兜底剥离其它类型前缀（例如（1））
            rest = stripAnyNumberingPrefix(title, itemCount)
            stripped = rest !== trimmedTitle
        }
        rest = rest.trimStart()
        if (!rest) return title
        if (!stripped && isIncompleteCompoundNumberingPrefix(title, paragraphCount, itemCount)) return title
        return `${chapterNum}${style.between}${itemNum}${style.after}${style.space}${rest}`
    }

    const expectedNum = formatOrdinal(itemIndex + 1, style.numeral)

    if (style.kind === 'paren') {
        const re = new RegExp(
            `^\\s*${escapeRegExp(style.open)}\\s*(\\d+|[零一二三四五六七八九十百千]+)\\s*${escapeRegExp(style.close)}\\s*([\\s\\S]*)$`
        )
        const trimmedTitle = title.trimStart()
        const m = title.match(re)
        let rest = trimmedTitle
        let stripped = false
        if (m && isLikelyIndexToken(m[1], itemCount)) {
            rest = m[2]
            stripped = true
        } else {
            rest = stripAnyNumberingPrefix(title, itemCount)
            stripped = rest !== trimmedTitle
        }
        rest = rest.trimStart()
        if (!rest) return title
        if (!stripped && isIncompleteNumberingPrefix(title, itemCount)) return title
        return `${style.open}${expectedNum}${style.close}${style.space}${rest}`
    }

    if (style.kind === 'delim') {
        const re = new RegExp(
            `^\\s*(\\d+|[零一二三四五六七八九十百千]+)\\s*${escapeRegExp(style.delim)}\\s*([\\s\\S]*)$`
        )
        const trimmedTitle = title.trimStart()
        const m = title.match(re)
        let rest = trimmedTitle
        let stripped = false
        if (m && isLikelyIndexToken(m[1], itemCount)) {
            rest = m[2]
            stripped = true
        } else {
            rest = stripAnyNumberingPrefix(title, itemCount)
            stripped = rest !== trimmedTitle
        }
        rest = rest.trimStart()
        if (!rest) return title
        if (!stripped && isIncompleteNumberingPrefix(title, itemCount)) return title
        return `${expectedNum}${style.delim}${style.space}${rest}`
    }

    if (style.kind === 'space') {
        const re = /^\s*(\d+|[零一二三四五六七八九十百千]+)\s+([\s\S]*)$/
        const trimmedTitle = title.trimStart()
        const m = title.match(re)
        let rest = trimmedTitle
        let stripped = false
        if (m && isLikelyIndexToken(m[1], itemCount)) {
            rest = m[2]
            stripped = true
        } else {
            rest = stripAnyNumberingPrefix(title, itemCount)
            stripped = rest !== trimmedTitle
        }
        rest = rest.trimStart()
        if (!rest) return title
        if (!stripped && isIncompleteNumberingPrefix(title, itemCount)) return title
        return `${expectedNum}${style.space}${rest}`
    }

    return title
}

const renumberOutlineTitlesIfNeeded = () => {
    const paragraphs = Paragraphs.value
    if (!Array.isArray(paragraphs) || paragraphs.length === 0) return

    const level1Style = detectLevel1Style(paragraphs)
    const shouldNumberLevel1 = !!level1Style

    for (let pIndex = 0; pIndex < paragraphs.length; pIndex++) {
        const p = paragraphs[pIndex]
        if (!p) continue

        if (shouldNumberLevel1 && typeof p.title === 'string' && p.title.trim()) {
            const nextTitle = applyLevel1Numbering(p.title, pIndex, level1Style, paragraphs.length)
            if (nextTitle !== p.title) p.title = nextTitle
        }

        const items = Array.isArray(p.content) ? p.content : []
        const level2Style = detectLevel2Style(items)
        if (!level2Style) continue

        for (let i = 0; i < items.length; i++) {
            const it = items[i]
            if (!it || typeof it.title !== 'string' || !it.title.trim()) continue
            const nextItemTitle = applyLevel2Numbering(it.title, pIndex, i, level2Style, paragraphs.length, items.length)
            if (nextItemTitle !== it.title) it.title = nextItemTitle
        }
    }
}

const handleOutlineTitleEditComplete = (newTitle) => {
    emit('updateOutline', newTitle, Paragraphs.value)
}

// 处理输入框获得焦点
const isInputFocused = ref(false)
const handleIsInputFocus = (value) => {
    // 使用nextTick确保DOM更新完成后再触发父组件更新
    nextTick(() => {
        isInputFocused.value = value
    })
}

// 修改来源跟踪
const changeSource = ref('')

// 监听Paragraphs变化
watch(Paragraphs, (newParagraphs) => {
    // 根据修改来源执行不同的逻辑
    switch(changeSource.value) {
        case 'updateContent':
        case 'updateTitle':
        case 'updateDescription':
        case 'deleteItem':
        case 'deleteParagraph':
        case 'addParagraph':
        case 'reorderParagraphs':
            // TODO: 更新内存中的段落数据
            emit('updateOutline', Title.value, Paragraphs.value)
            break
        default:
            break
    }
    
    // 重置修改来源
    changeSource.value = ''
   
}, {immediate: true, deep: true})

// 添加段落
const addParagraph = async () => {
    // 如果组件被禁用，则不执行添加操作
    if (props.disabled) {
        return;
    }
    
    changeSource.value = 'addParagraph'
    Paragraphs.value.push({
        title: '',
        content: []
    })

    // 新增/删除/重排会改变序号，需要重算
    renumberOutlineTitlesIfNeeded()
    
    // 等待DOM更新完成
    await nextTick()
    
    // 聚焦到新添加的段落的标题输入框
    focusNewParagraphTitleInput(Paragraphs.value.length - 1)
}

// 更新段落内容
const updateParagraphContent = (paragraphIndex, newContent) => {
    changeSource.value = 'updateContent'
    // 使用数组的splice方法来触发Vue的响应式更新，但避免整个数组重新渲染
    const paragraph = Paragraphs.value[paragraphIndex]
    if (paragraph) {
        paragraph.content = newContent
    }

    // 子标题增删/拖拽重排后需要重算序号（尤其是 1.1 这类前缀）
    renumberOutlineTitlesIfNeeded()
}

// 更新段落标题
const updateParagraphTitle = (paragraphIndex, newTitle) => {
    changeSource.value = 'updateTitle'
    const paragraph = Paragraphs.value[paragraphIndex]
    if (paragraph) {
        paragraph.title = newTitle
    }

    // 章节标题编辑后也需要校准前缀序号
    renumberOutlineTitlesIfNeeded()
}

// 更新段落描述
const updateParagraphDescription = (paragraphIndex, newDescription) => {
    changeSource.value = 'updateDescription'
    const paragraph = Paragraphs.value[paragraphIndex]
    if (paragraph) {
        paragraph.description = newDescription
    }
}

// 段落拖拽状态
const paragraphDragState = ref({
    isDragging: false,
    draggedIndex: -1,
    placeholderIndex: -1
})

// Outline组件根元素引用
const outlineRef = ref(null)

const paragraphDragOverIndex = ref(-1)
const paragraphDragDirection = ref('') // 添加拖拽方向状态

// 自动滚动相关状态 - 预留接口方便调试
const autoScrollInterval = ref(null)
// 自动滚动配置
const autoScrollConfig = {
    edgeThreshold: 40, // 边缘阈值（像素）
    scrollSpeed: 200, // 滚动速度（像素/秒）
    scrollInterval: 16 // 滚动间隔（毫秒）
}

// 处理段落拖拽开始
const handleParagraphDragStart = (dragData) => {
    paragraphDragState.value = {
        isDragging: true,
        draggedIndex: dragData.index,
        placeholderIndex: -1,
        startY: dragData.startY,
        elementTop: dragData.elementTop,
        draggedHeight: dragData.originalHeight || dragData.element.offsetHeight,
        draggedWidth: dragData.originalWidth || dragData.element.offsetWidth,
        mouseOffsetY: dragData.mouseOffsetY || 0
    }
}



// 检查是否需要自动滚动
const checkAutoScroll = (mouseY) => {
    const chatHistory = document.getElementById('chatHistory')
    if (!chatHistory) return
    
    const chatHistoryRect = chatHistory.getBoundingClientRect()
    const topEdge = chatHistoryRect.top + autoScrollConfig.edgeThreshold
    const bottomEdge = chatHistoryRect.bottom - autoScrollConfig.edgeThreshold
    
    // 清除之前的自动滚动
    if (autoScrollInterval.value) {
        clearInterval(autoScrollInterval.value)
        autoScrollInterval.value = null
    }
    
    if (mouseY <= topEdge) {
        // 鼠标在顶部边缘，向上滚动
        startAutoScroll(-1)
    } else if (mouseY >= bottomEdge) {
        // 鼠标在底部边缘，向下滚动
        startAutoScroll(1)
    } else {
        // 鼠标在中间区域，停止滚动
        stopAutoScroll()
    }
}

// 处理段落拖拽移动
const handleParagraphDragMove = (dragData) => {
    if (!paragraphDragState.value.isDragging) return
    
    const currentY = dragData.currentY || dragData.event.clientY
    const paragraphElements = document.querySelectorAll('.outline-paragraph-container')
    const draggedHeight = paragraphDragState.value.draggedHeight
    
    let newIndex = paragraphDragState.value.draggedIndex
    
    // 获取当前拖拽元素的实际位置（考虑鼠标偏移量）
    const mouseOffsetY = paragraphDragState.value.mouseOffsetY || 0
    const draggedTop = currentY - mouseOffsetY
    const draggedBottom = draggedTop + draggedHeight
    
    // 检查是否需要自动滚动
    checkAutoScroll(currentY)
    
    for (let i = 0; i < paragraphElements.length; i++) {
        if (i === paragraphDragState.value.draggedIndex) continue
        
        const rect = paragraphElements[i].getBoundingClientRect()
        const itemTop = rect.top
        const itemBottom = rect.top + rect.height
        
        // 改进的边界检测：当拖拽元素的上边界或下边界越过相邻元素时触发交换
        if (i < paragraphDragState.value.draggedIndex) {
            // 向上移动：当拖拽元素的上边界越过目标元素的上边界时
            if (draggedTop < itemTop + 10) { // 添加10px的缓冲区域
                newIndex = i
                paragraphDragDirection.value = 'up'
                break
            }
        }
        else if (i > paragraphDragState.value.draggedIndex) {
            // 向下移动：当拖拽元素的下边界越过目标元素的下边界时
            if (draggedBottom > itemBottom - 10) { // 添加10px的缓冲区域
                newIndex = i
                paragraphDragDirection.value = 'down'
            }
        }
    }
    
    if (newIndex !== paragraphDragState.value.draggedIndex && newIndex !== paragraphDragState.value.placeholderIndex) {
        paragraphDragState.value.placeholderIndex = newIndex
        paragraphDragOverIndex.value = newIndex
    }
}

// 处理段落拖拽结束
const handleParagraphDragEnd = (dragData) => {
    // 停止自动滚动
    stopAutoScroll()
    
    // 在拖拽结束时执行重排
    if (paragraphDragState.value.placeholderIndex !== -1 && paragraphDragState.value.placeholderIndex !== paragraphDragState.value.draggedIndex) {
        reorderParagraphs(paragraphDragState.value.draggedIndex, paragraphDragState.value.placeholderIndex)
    }
    
    paragraphDragState.value = {
        isDragging: false,
        draggedIndex: -1,
        placeholderIndex: -1
    }
    paragraphDragOverIndex.value = -1
    paragraphDragDirection.value = '' // 重置拖拽方向
}

// 重排段落
const reorderParagraphs = (fromIndex, toIndex) => {
    if (fromIndex === toIndex) return
    
    changeSource.value = 'reorderParagraphs'
    
    // 直接操作原数组以触发响应式更新
    const paragraphs = Paragraphs.value
    const draggedParagraph = paragraphs[fromIndex]
    
    // 移除原位置的元素
    paragraphs.splice(fromIndex, 1)
    // 在新位置插入元素
    paragraphs.splice(toIndex, 0, draggedParagraph)

    // 重排会改变序号，需要重算
    renumberOutlineTitlesIfNeeded()
}



// 开始自动滚动
const startAutoScroll = (direction) => {
    if (autoScrollInterval.value) return
    
    autoScrollInterval.value = setInterval(() => {
        const chatHistory = document.getElementById('chatHistory')
        if (chatHistory) {
            const scrollAmount = (autoScrollConfig.scrollSpeed * autoScrollConfig.scrollInterval) / 1000
            chatHistory.scrollTop += scrollAmount * direction
        }
    }, autoScrollConfig.scrollInterval)
}

// 停止自动滚动
const stopAutoScroll = () => {
    if (autoScrollInterval.value) {
        clearInterval(autoScrollInterval.value)
        autoScrollInterval.value = null
    }
}

// 更新自动滚动配置（调试用）
const updateAutoScrollConfig = (config) => {
    if (config.scrollSpeed !== undefined) {
        autoScrollConfig.scrollSpeed = config.scrollSpeed
        console.log(`自动滚动速度已更新为: ${autoScrollConfig.scrollSpeed}px/s`)
    }
    if (config.edgeThreshold !== undefined) {
        autoScrollConfig.edgeThreshold = config.edgeThreshold
        console.log(`自动滚动触发阈值已更新为: ${autoScrollConfig.edgeThreshold}px`)
    }
    if (config.scrollInterval !== undefined) {
        autoScrollConfig.scrollInterval = config.scrollInterval
        console.log(`自动滚动间隔已更新为: ${autoScrollConfig.scrollInterval}ms`)
    }
    return {
        edgeThreshold: autoScrollConfig.edgeThreshold,
        scrollSpeed: autoScrollConfig.scrollSpeed,
        scrollInterval: autoScrollConfig.scrollInterval
    }
}



// 聚焦到新添加段落的标题输入框
const focusNewParagraphTitleInput = async (paragraphIndex) => {
    // 等待DOM完全更新
    await nextTick()
    
    // 只在当前Outline组件内查找段落
    const outlineContainer = outlineRef.value
    if (!outlineContainer) return
    
    // 获取所有段落容器
    const paragraphContainers = outlineContainer.querySelectorAll('.outline-paragraph-container')
    if (paragraphIndex >= 0 && paragraphIndex < paragraphContainers.length) {
        const targetContainer = paragraphContainers[paragraphIndex]
        // 查找段落标题的输入框 - 使用OutlineInputArea组件
        const titleInputArea = targetContainer.querySelector('.outline-paragraph-title-text .input-field')
        if (titleInputArea) {
            titleInputArea.focus()
            
            // 添加输入事件监听器来检查空标题
            const checkEmptyTitle = () => {
                if (titleInputArea.value.trim() === '') {
                    // 如果标题为空，删除该段落
                    handleDeleteParagraph(paragraphIndex)
                }
            }
            
            // 在失去焦点时检查
            titleInputArea.addEventListener('blur', checkEmptyTitle, { once: true })
            
            // 也监听输入事件，如果用户开始输入则移除blur监听器
            const removeBlurListener = () => {
                if (titleInputArea.value.trim() !== '') {
                    titleInputArea.removeEventListener('blur', checkEmptyTitle)
                    titleInputArea.removeEventListener('input', removeBlurListener)
                }
            }
            titleInputArea.addEventListener('input', removeBlurListener)
        }
    }
}

// 删除outline-item
const handleDeleteItem = (paragraphIndex, itemIndex) => {
    if (paragraphIndex >= 0 && paragraphIndex < Paragraphs.value.length) {
        const paragraph = Paragraphs.value[paragraphIndex]
        if (paragraph && paragraph.content && itemIndex >= 0 && itemIndex < paragraph.content.length) {
            changeSource.value = 'deleteItem'
            // 使用数组的splice方法来删除指定索引的item
            paragraph.content.splice(itemIndex, 1)

            // 删除会导致后续序号变化
            renumberOutlineTitlesIfNeeded()
        }
    }
}

// 删除整个段落
const handleDeleteParagraph = (paragraphIndex) => {
    if (paragraphIndex >= 0 && paragraphIndex < Paragraphs.value.length) {
        changeSource.value = 'deleteParagraph'
        // 使用数组的splice方法来删除指定索引的段落
        Paragraphs.value.splice(paragraphIndex, 1)

        // 删除会导致后续序号变化
        renumberOutlineTitlesIfNeeded()
    }
}

// 在组件挂载后设置全局调试接口
onMounted(() => {
    // 将组件实例暴露到全局，方便调试
    window.OutlineDebug = {
        updateAutoScrollConfig: updateAutoScrollConfig,
        getCurrentConfig: () => ({
            edgeThreshold: autoScrollConfig.edgeThreshold,
            scrollSpeed: autoScrollConfig.scrollSpeed,
            scrollInterval: autoScrollConfig.scrollInterval
        })
    }
})

// 在组件卸载时清理
onUnmounted(() => {
    stopAutoScroll()
    if (window.OutlineDebug) {
        delete window.OutlineDebug
    }
})


</script>

<style scoped lang="scss">
.outline {
    display: flex;
    flex-direction: column;
    margin: 10px; 
    // background-color: #f5f5f5;
    width: calc(100% - 20px); // 确保占满可用宽度
    max-width: 100%; // 确保不超过父容器宽度
    box-sizing: border-box; // 确保宽度计算包含padding和border

    .outline-title {
        margin-bottom: 10px;
        border: 1px solid var(--uosai-color-outline-paragraph-border);
        background-color: var(--uosai-color-outline-paragraph-bg);
        border-radius: 8px;

        &.focused {
            border-color: var(--activityColor);
        }
    }

    .outline-paragraph-wrapper {
        transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
        position: relative;
        width: 100%;
        box-sizing: border-box;
        
        &.drag-over {
            position: relative;
            
            &.drag-over-before {
                margin-top: 80px; // 为拖拽段落预留顶部空间
            }
            
            &.drag-over-after {
                margin-bottom: 80px; // 为拖拽段落预留底部空间
            }
            
            opacity: 0.7;
            
            // 添加视觉指示器
            &::before {
                content: '';
                position: absolute;
                left: 0;
                right: 0;
                height: 3px;
                background-color: var(--activityColor);
                z-index: 100;
            }
            
            &.drag-over-before::before {
                top: -40px;
            }
            
            &.drag-over-after::before {
                bottom: -40px;
            }
        }
        
        &.dragging {
            opacity: 1;
        }
    }

    .outline-add {
        padding: 10px;
        color: var(--uosai-color-outline-item-text);
        border: 1px solid var(--uosai-color-outline-paragraph-border);
        background-color: var(--uosai-color-outline-paragraph-bg);
        font-size: 1rem;
        font-weight: 500;
        cursor: pointer;
        display: flex;
        align-items: center;
        border-radius: 8px;

        .outline-add-text{
            margin-left: 10px;
            display: flex;
            align-items: center;
        }
        
        &:hover {
            background-color: var(--uosai-color-outline-add-hover-bg);
        }

        &:active {
            background-color: var(--uosai-color-outline-add-active-bg);
        }
    }
    
    // 禁用状态样式
    &.disabled {
        opacity: 0.6;
        pointer-events: none;
        user-select: none;
        
        .outline-paragraph-wrapper {
            opacity: 0.6;
        }
        
        .outline-add {
            opacity: 0.4;
            cursor: not-allowed;
            
            &:hover {
                background-color: transparent;
            }
        }
    }
}
</style>
