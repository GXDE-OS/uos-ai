---
name: 安装技能
description: 当用户想安装、导入或添加 skill/技能，或提供本地 skill 目录、SKILL.md、.skill/.zip/.tar.* 压缩包、http(s) 下载链接时使用。聊天环境中优先调用 install_skill 工具完成安装。
---

# 安装技能

当用户要安装或导入 skill 时，先确认来源类型：

- 本地 skill 目录
- 本地 `SKILL.md`
- 本地 `.skill`、`.zip`、`.tar`、`.tar.gz`、`.tgz`、`.tar.bz2`、`.tar.xz` 压缩包
- 直接可下载的 `http` 或 `https` URL

## UOS AI Skill 存储路径

如果聊天或工具上下文提供了 `<UOS_AI_SKILL_PATHS>`，必须优先使用其中的 `User skill install path` 作为 UOS AI 的用户级 Skill 安装目录。如果没有该上下文，默认安装目录是：

```text
$HOME/.uos-ai/skills
```

安装成功后，Skill 会位于该目录下的 `<skillName>` 子目录中。不要把 `$XDG_SKILLS_HOME`、`$HOME/.claude/skills` 或 `$HOME/.agents/skills` 当作 UOS AI 的默认安装目标，除非用户明确要求操作这些外部生态目录。

如果当前聊天环境提供 `install_skill` 工具，优先直接调用它：

```json
{"source":"<path-or-url>"}
```

如果没有 `install_skill` 工具，再使用 UOS AI 原生命令作为备用方式：

```bash
uos-ai-cli skill-add "<path-or-url>"
```

在 TUI 中也可以执行：

```text
:skill-add <path-or-url>
```

安装后用以下方式确认；确认时应看到该 Skill 来源为 UOS AI 用户安装目录对应的来源：

```bash
uos-ai-cli skills
```

或在 TUI 中执行：

```text
:skills
```

注意：

- URL 必须是直接下载链接，指向 `SKILL.md` 或 skill 压缩包。
- 如果用户提供的是普通网页展示地址，而不是 raw/archive 下载地址，请要求用户提供直接下载链接。
- 只安装用户信任来源的 skill。
- 如果安装失败，向用户展示原始错误信息，并建议检查 `SKILL.md` front matter 是否包含 `name` 字段。
