/**
 * FileEvent enum matching C++ FileEvent definition
 */
export enum FileEvent {
    Unknown = 0,
    FeFileReady = 1, // file validated, frontend should show file card
    FeParseResult = 2, // async parse complete
    FeNativeDrop = 3, // native webview drop event with file paths and coordinates
    FeIncomingFiles = 4, // backend hands paths back to frontend for unified constraint checks
}

export enum FileCategory {
    Material = 0,
    Outline = 1,
}

export interface SelectFileOptions {
    multiple?: boolean;
    pluginOnly?: boolean;
    category?: FileCategory;
}

/**
 * Upload file item interface
 */
export interface UploadFile {
    /** Unique identifier for the file */
    id: string;
    /** File path on the system */
    filePath: string;
    /** File name */
    fileName: string;
    /** File size in bytes */
    fileSize?: number;
    /** File icon as base64 string */
    icon?: string;
    /** Default prompt for the file */
    defaultPrompt?: string;
    /** Parse status */
    parseStatus: "pending" | "parsing" | "completed" | "error";
    /** Error code, 0 means no error */
    error: number;
    /** Parse result content */
    parseResult?: string;
    /** File category */
    category?: FileCategory;
}

export interface NativeDropPayload {
    paths: string[];
    x: number;
    y: number;
}
