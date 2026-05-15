/**
 * File parsing file type enum
 */
export enum DocParsingFileType {
    Doc = 0, // Document
    Image = 1, // Image
}

/**
 * File category enum
 */
export enum DocFileCategory {
    LocalMaterial = 0, // Local material
    FileOutline = 1, // File outline
}

/**
 * File alignment enum
 */
export enum FileAlignment {
    Left = "left", // Align left
    Right = "right", // Align right
}

/**
 * Popover placement direction enum
 */
export enum PopoverPlacement {
    Top = "top", // Show above button
    Bottom = "bottom", // Show below button
}

/**
 * Popover alignment enum
 */
export enum PopoverAlign {
    Left = "left", // Align left edge with button
    Right = "right", // Align right edge with button
    Center = "center", // Align center with button
}

/**
 * File parsing status
 */
export type FileParseStatus = "pending" | "parsing" | "completed" | "error";

/**
 * File item interface for displaying in chat
 */
export interface DisplayFile {
    index: number;
    fileNameText: string;
    filePath: string;
    type: DocParsingFileType;
    isExist?: boolean;
    category?: DocFileCategory;
    imgBase64?: string;
    /** File size for original data */
    fileSize?: number;
    /** File type text for display (e.g., "Word", "PDF") */
    fileType?: string;
    /** File size text for display (e.g., "2.5 MB") */
    fileSizeText?: string;
    /** Parse status for file upload state */
    parseStatus?: FileParseStatus;
}
