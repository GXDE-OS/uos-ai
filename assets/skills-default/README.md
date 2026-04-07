# System Default Skills

This directory contains pre-installed skills that will be available to all users by default.

## Directory Structure

Each subdirectory represents one skill and must contain at least a `SKILL.md` file with proper front matter:

```
skills-default/
├── skill-name-1/
│   └── SKILL.md
├── skill-name-2/
│   └── SKILL.md
└── skill-name-3/
    └── SKILL.md
```

## Skill Format

Each `SKILL.md` file should follow this format:

```markdown
---
name: skill-name
description: Brief description of what this skill does
---

Detailed instructions for the skill...
```

## Installation

Skills in this directory are installed to `/usr/lib/uos-ai-assistant/skills-default/` during the build process.

## Priority

Skills are loaded with the following priority (higher priority overrides lower):

1. `~/.uos-ai/skills/` (highest priority - user-specific)
2. Custom paths from `~/.config/deepin/uos-ai-assistant/skills.conf`
3. `/usr/lib/uos-ai-assistant/skills-default/` (lowest priority - system default)

This means users can override system default skills by placing a skill with the same name in their own `~/.uos-ai/skills/` directory.
