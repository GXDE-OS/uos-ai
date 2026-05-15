/**
 * 将时间戳格式化为 yyyy-mm-dd 格式
 * @param timestamp 时间戳（毫秒）
 * @returns 格式化后的日期字符串
 */
export function formatDate(timestamp: number): string {
    const date = new Date(timestamp);
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, "0");
    const day = String(date.getDate()).padStart(2, "0");
    return `${year}-${month}-${day}`;
}

/**
 * 判断给定时间戳是否为今天
 * @param timestamp 时间戳（毫秒）
 * @returns 是否为今天
 */
export function isToday(timestamp: number): boolean {
    const date = new Date(timestamp);
    const today = new Date();
    return (
        date.getFullYear() === today.getFullYear() &&
        date.getMonth() === today.getMonth() &&
        date.getDate() === today.getDate()
    );
}

/**
 * 判断给定时间戳是否为昨天
 * @param timestamp 时间戳（毫秒）
 * @returns 是否为昨天
 */
export function isYesterday(timestamp: number): boolean {
    const date = new Date(timestamp);
    const yesterday = new Date();
    yesterday.setDate(yesterday.getDate() - 1);
    return (
        date.getFullYear() === yesterday.getFullYear() &&
        date.getMonth() === yesterday.getMonth() &&
        date.getDate() === yesterday.getDate()
    );
}

/**
 * 判断给定时间戳是否在过去7天内（不包括今天和昨天）
 * @param timestamp 时间戳（毫秒）
 * @returns 是否在过去7天内
 */
export function isPast7Days(timestamp: number): boolean {
    const date = new Date(timestamp);
    const now = new Date();
    const sevenDaysAgo = new Date(now.getTime() - 7 * 24 * 60 * 60 * 1000);
    return date >= sevenDaysAgo && !isToday(timestamp) && !isYesterday(timestamp);
}

/**
 * 判断给定时间戳是否在过去30天内（不包括过去7天）
 * @param timestamp 时间戳（毫秒）
 * @returns 是否在过去30天内
 */
export function isPast30Days(timestamp: number): boolean {
    const date = new Date(timestamp);
    const now = new Date();
    const thirtyDaysAgo = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);
    return date >= thirtyDaysAgo && !isPast7Days(timestamp) && !isToday(timestamp) && !isYesterday(timestamp);
}

/**
 * 判断给定时间戳是否在今年内（不包括过去30天）
 * @param timestamp 时间戳（毫秒）
 * @returns 是否在今年内
 */
export function isThisYear(timestamp: number): boolean {
    const date = new Date(timestamp);
    const now = new Date();
    return (
        date.getFullYear() === now.getFullYear() &&
        !isPast30Days(timestamp) &&
        !isPast7Days(timestamp) &&
        !isToday(timestamp) &&
        !isYesterday(timestamp)
    );
}

/**
 * 判断给定时间戳是否在过去5年内（不包括今年）
 * @param timestamp 时间戳（毫秒）
 * @returns 是否在过去5年内
 */
export function isPast5Years(timestamp: number): boolean {
    const date = new Date(timestamp);
    const now = new Date();
    const fiveYearsAgo = new Date(now.getFullYear() - 5, 0, 1);
    return date >= fiveYearsAgo && !isThisYear(timestamp);
}

/**
 * 用毫秒时间戳+随机数创建id，格式为 毫秒时间戳_4位随机数
 * @returns 生成的id
 */
export function createId(): string {
    const timestamp = Date.now();
    const randomNum = Math.floor(Math.random() * 9000) + 1000; // 生成1000-9999的随机数
    return `${timestamp}_${randomNum}`;
}
