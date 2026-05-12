#ifndef RESEARCH_KEY_DEFINE_H
#define RESEARCH_KEY_DEFINE_H

namespace uos_ai {

// Tool names
inline constexpr char STR_RESEARCH_TOOL_WEB_SEARCH[]    = "web_search";
inline constexpr char STR_RESEARCH_TOOL_LOCAL_SEARCH[]  = "local_search";
inline constexpr char STR_RESEARCH_TOOL_PARSE_FILE[]    = "parse_upload_file";
inline constexpr char STR_RESEARCH_TOOL_RETRIEVE[]      = "retrieve";
inline constexpr char STR_RESEARCH_TOOL_RENAME_ARTICLE[]  = "rename_article";
inline constexpr char STR_RESEARCH_TOOL_RENAME_SECTION[]  = "rename_section";
inline constexpr char STR_RESEARCH_TOOL_EDIT_SECTION[]    = "edit_section";
inline constexpr char STR_RESEARCH_TOOL_REWRITE_ARTICLE[] = "rewrite_article";

// Stage params
inline constexpr char STR_RESEARCH_OUTLINE_PATH[]   = "outline_path";
inline constexpr char STR_RESEARCH_UPLOAD_FILES[]   = "upload_files";
inline constexpr char STR_RESEARCH_FILES[]          = "files";

// Retrieve tool params
inline constexpr char STR_RESEARCH_REF_IDS[]   = "ref_ids";
inline constexpr char STR_RESEARCH_GOAL[]      = "goal";

// Content fields
inline constexpr char STR_RESEARCH_ARTICLE_CONTENT[] = "article_content";
inline constexpr char STR_RESEARCH_CLEAN_ARTICLE[]   = "clean_article";
inline constexpr char STR_RESEARCH_CLEANED_CONTENT[] = "cleaned_content";
inline constexpr char STR_RESEARCH_SECTION_ID[]      = "section_id";
inline constexpr char STR_RESEARCH_DELETE_SECTION[]  = "DELETE_SECTION";

// Article model fields
inline constexpr char STR_RESEARCH_LEVEL[]    = "level";
inline constexpr char STR_RESEARCH_SECTIONS[] = "sections";

// Article & version fields
inline constexpr char STR_RESEARCH_ARTICLES[]         = "articles";
inline constexpr char STR_RESEARCH_ARTICLE_IDS[]      = "article_ids";
inline constexpr char STR_RESEARCH_ACTIVE_ARTICLE_ID[] = "active_article_id";
inline constexpr char STR_RESEARCH_VERSIONS[]         = "versions";
inline constexpr char STR_RESEARCH_CURRENT_VERSION[]  = "current_version";
inline constexpr char STR_RESEARCH_VERSION[]          = "version";
inline constexpr char STR_RESEARCH_CHANGE_DESC[]      = "change_description";

// Export format
inline constexpr char STR_RESEARCH_FORMAT_MD[]   = "md";
inline constexpr char STR_RESEARCH_FORMAT_DOCX[] = "docx";
inline constexpr char STR_RESEARCH_FORMAT_PDF[]  = "pdf";

} // namespace uos_ai

#endif // RESEARCH_KEY_DEFINE_H
