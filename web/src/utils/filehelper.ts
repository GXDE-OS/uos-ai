import type { DisplayFile } from "@/types/file";
import { DocFileCategory } from "@/types/file";

/**
 * Format file size to human readable string
 * @param bytes File size in bytes
 * @returns Formatted file size string (e.g., "1.5 MB", "256 KB", "0B", "-")
 */
export const formatFileSize = (bytes: number | undefined): string => {
    if (bytes === undefined) return "-";
    if (bytes === 0) return "0B";

    const units = ["B", "KB", "MB", "GB"];
    let size = bytes;
    let unitIndex = 0;

    while (size >= 1024 && unitIndex < units.length - 1) {
        size /= 1024;
        unitIndex++;
    }

    // Use 1 decimal place for MB and larger, no decimals for bytes
    if (unitIndex === 0) {
        return `${size} ${units[unitIndex]}`;
    } else {
        return `${size.toFixed(1)} ${units[unitIndex]}`;
    }
};

/**
 * Get file type from file name extension
 * @param fileName File name
 * @param translate Translation function
 * @returns File type string (e.g., "Word", "PPT", "PDF")
 */
export const getFileType = (fileName: string, translate?: (key: string) => string): string => {
    if (!fileName) return "";

    const lastDotIndex = fileName.lastIndexOf(".");
    if (lastDotIndex === -1) return "";

    const ext = fileName.substring(lastDotIndex + 1).toLowerCase();

    const typeMap: Record<string, string> = {
        docx: "Word",
        doc: "Word",
        wps: "WPS",
        dps: "WPS",
        et: "WPS",
        pptx: "PPT",
        ppt: "PPT",
        xlsx: "Excel",
        xls: "Excel",
        csv: "CSV",
        pdf: "PDF",
        md: "Markdown",
        txt: translate?.("Document") || "Document",
        rtf: translate?.("Document") || "Document",
    };

    return typeMap[ext] || ext;
};

/**
 * Get a compact category label for writing assistant files.
 * Outline files should display "大纲" in Chinese, and materials should display "素材".
 */
export const getFileCategoryLabel = (
    file: Pick<DisplayFile, "category">,
    translate?: (key: string) => string,
): string => {
    if (file.category === DocFileCategory.FileOutline) {
        const outlineLabel = translate?.("Outline") || "Outline";
        return outlineLabel.includes("大纲") ? "大纲" : outlineLabel;
    }

    if (file.category === DocFileCategory.LocalMaterial) {
        const materialLabel = translate?.("Local Materials") || "Local Materials";
        if (materialLabel.includes("素材")) {
            return "素材";
        }
        return materialLabel === "Local Materials" ? "Material" : materialLabel;
    }

    return "";
};

/**
 * Keep upload order stable while forcing outline files to the front for writing flows.
 */
export const sortDisplayFilesForGroup = (files: DisplayFile[]): DisplayFile[] => {
    const hasWritingCategory = files.some(
        (file) =>
            file.category === DocFileCategory.FileOutline || file.category === DocFileCategory.LocalMaterial,
    );

    if (!hasWritingCategory) {
        return files;
    }

    const outlineFiles: DisplayFile[] = [];
    const otherFiles: DisplayFile[] = [];

    files.forEach((file) => {
        if (file.category === DocFileCategory.FileOutline) {
            outlineFiles.push(file);
            return;
        }
        otherFiles.push(file);
    });

    return [...outlineFiles, ...otherFiles];
};
