# UOS AI|uos-ai-assistant|

## Overview

UOS AI is an integrated AI assistant. In the current version, its main capabilities include **AI Writing**, **AI Knowledge Base**, **AI Translation**, and **MCP&Skills**, providing a unified AI-assisted experience.

In this version, users mainly work with the main window and the settings window. The main window is used for conversations, assistant switching, and history access. The settings window is used to manage models, the knowledge base, MCP, FollowAlong, IM integration, and proxy options.

## Quick Start

### Main Window

The current UOS AI main window is composed of the left navigation area, the central content area, and the input area at the bottom. The left side is used to switch feature entries, the center displays the welcome page, conversation content, or history content, and the bottom area is used to enter and send requests.

![image-20260508174703006](fig/image-20260508174703006.png)

The model selector in the upper-left corner currently shows **Intelligent Routing**. Click it to open the model list, switch models, or enter the model adding flow.

![image-20260508174715964](fig/image-20260508174715964.png)

The current dropdown includes:

- **Online Model**: opens the online model entry.
- **Intelligent Routing**: switches back to the current routing mode.
- **Add Model**: opens the model adding flow.

### Left Navigation

The left side of the main window currently includes these main entries:

**New Chat**

Starts a new session and returns to the welcome page.

**AI Writing**

Opens the writing page for reports, notices, summaries, outlines, and similar structured content.

**AI Knowledge Base**

Used for question answering based on imported knowledge base files. Files must be added first in **Knowledge Base Management**.

**MCP&Skills**

Used to call built-in or enabled MCP services and to manage Skills.

**AI Translation**

Used for bilingual translation, formal rewording, and short text translation tasks.

**More**

Expands additional entries. In the current UI, **Agent Store** is shown there.

**Agent Store**

Located under the expanded **More** section and used to open the Agent Store entry.

**Chat History**

Shows the current session title list.

**Chat History management button**

Located on the right side of **Chat History** and used to open the history page.

### Input Area

The input area at the bottom is the main action area of the window.

- **Input box**: used to enter questions, writing prompts, or translation requests.
- **DeepThink**: turns deep reasoning on or off.
- **Search**: turns online search on or off.
- **Attachment entry**: used to add supplementary content.
- **Microphone**: used for voice input when available.
- **Send button**: sends the current input.

## Assistants

### UOS AI Assistant

UOS AI is the default general-purpose assistant. It can be used for general Q&A, rewriting, writing assistance, and follow-up questions in the same conversation.

During normal use:

1. Enter a request in the input area and click **Send**.
2. After the request is submitted, the interface enters a running state, and the send button switches to **Stop**.
3. After the response is generated, the result remains in the current session and follow-up prompts can continue there.

### Running-State Interaction

During actual use, users most often interact with the interface after a request has already been sent. In the current version:

1. After entering content in the input area, click **Send** to submit it. Some scenes also support direct submission with Enter.
2. After the request is sent, the interface enters a running state. Common states include reasoning, search, tool calls, and answer generation.
3. While the assistant is still generating content, the send button switches to **Stop**, allowing the current task to be interrupted.
4. After generation is complete, the current session remains open so the user can continue with follow-up requests.
5. Both in-progress content and finished results remain in the conversation and can later be reopened from the history list.

### AI Writing

AI Writing is used to generate structured documents. In the current version, it is easiest to understand in three stages: before writing, during writing, and after writing.

#### Before Writing

After entering **AI Writing**, the input area switches to a writing template with variable placeholders. Users can fill in the template directly or enter a natural-language request.

![image-20260508174756636](fig/image-20260508174756636.png)

The current template includes fields such as identity, document type, topic, target length, and content requirements.

The attachment entry on the lower-right side of the input area can be used to open supplementary content sources. In the current version, the main sources are:

1. **Outline**: used to provide a chapter structure before generating the document.
2. **Local File**: used to upload local reference files so the generated content can use them as supporting material.

#### During Writing

After the writing request is sent, AI Writing first enters the outline generation flow. If the task is suitable for structured writing, the page first shows **Generating outline** and then produces a chapter-based outline draft.

1. After the writing request is submitted, the page enters the outline generation state.
2. After the outline is generated, a chapter structure appears on the left side.
3. Users can click **Generate document from outline** to continue into the document generation flow.

During outline editing, users can adjust chapter content directly:

1. Click the **plus** icon in the upper-right corner of a chapter to add a new item under that chapter.
2. Click the **delete** icon in the upper-right corner of a chapter to remove it.
3. The chapter order can be adjusted through the drag handle on the left side of each chapter.

![image-20260508174820523](fig/image-20260508174820523.png)

After **Generate document from outline** is clicked, the page continues through the main processing stages such as collecting materials and generating the document.

#### After Writing

After document generation is complete, the page enters an editable document stage. The left side keeps the outline and generation summary, while the right side shows the generated article in the editor.

![image-20260508174841358](fig/image-20260508174841358.png)

At this stage, users can:

1. Continue editing the generated article directly in the document area.
2. Use the top toolbar for common formatting operations.
3. Enter follow-up requirements in the bottom input area to refine the generated content.
4. Export the current document through the export menu.

Clicking the export entry on the document card opens the export menu. The current visible items include:

- **Save as PDF**
- **Save as Markdown**
- **Save as Word**

![6|writing export menu](fig/p06-writing-export-menu.png)

In the verified workflow, exporting to PDF enters the save flow and produces a local output file successfully.

### AI Knowledge Base

AI Knowledge Base is used for question answering based on imported documents.

After entering **AI Knowledge Base**, users can ask questions directly in the input area.

![image-20260508175000227](fig/image-20260508175000227.png)

If the knowledge base has not yet been configured, the page prompts the user to configure it first. If files have already been imported, the answer area can display both the response and its referenced source files.

![image-20260508175016724](fig/image-20260508175016724.png)

Based on the imported materials, the system can return a direct answer. For example, when asking when an expense report should be submitted after returning from a business trip, the current verified answer flow returns the result based on the imported reimbursement policy content.

### AI Translation

AI Translation is used for translation tasks.

After entering **AI Translation**, the page shows example cards. Users can click an example or enter their own translation request.

![image-20260508175038781](fig/image-20260508175038781.png)

In the verified English UI, entering a Chinese sentence such as “本周部署已经完成，请大家查看最新看板。” produces an English result successfully.

![image-20260508175046988](fig/image-20260508175046988.png)

### MCP&Skills

**MCP&Skills** is used to call extension capabilities and manage MCP services and Skills in one place.

The top area currently provides two entries:

1. **My MCP Services**
2. **My Skills**

The middle area displays sample capability cards, and the bottom input area is used to enter MCP&Skills instructions directly.

![image-20260508175100240](fig/image-20260508175100240.png)

In the verified workflow, after entering a request such as checking the current system time, the page enters a running state and returns the result. While the task is running, the send button switches to **Stop**.

Click **My MCP Services** to open the services list page. The currently visible services include:

- `uos-mcp`
- `12306-mcp`
- `excel`
- `fetch`
- `mcp-server-chart`
- `word-document-server`

Each service entry includes a switch on the right side for enabling or disabling the service.

![12|mcp services](fig/p12-mcp-services.png)

The filter dropdown in the upper-right corner currently includes:

- **All**
- **Built-in only**
- **Custom only**

![13|mcp filter](fig/p13-mcp-filter.png)

Click **My Skills** to open the Skills list page. In the current page, the upper-right area includes actions such as refreshing the list and importing a skill, while each skill entry includes an enable switch.

![14|skills page](fig/p14-skills-page.png)

## Chat History

Click the management button on the right side of **Chat History** to open the history page.

![15|history page](fig/p15-history.png)

On the history page:

1. The top **search** button is used to expand the conversation search field.
2. The **batch management** button is used to enter multi-selection mode.
3. The filter dropdown in the upper-right corner is used to switch the visible scope.
4. Clicking a conversation item reopens that conversation.

After search is expanded, the top area shows the search input field:

![16|history search](fig/p16-history-search.png)

After batch management is entered, the bottom area shows the batch action bar and supports selecting conversations:

![17|history batch](fig/p17-history-batch.png)

The current filter dropdown can be expanded to switch the visible history scope by assistant category:

![18|history filter](fig/p18-history-filter.png)

## Settings

Important configuration items in the current version are concentrated in the settings window. The left side of the settings window currently includes:

- **Model Configuration**
- **Online model**
- **Local model**
- **Private deployment model**
- **MCP Server**
- **UOS AI FollowAlong**
- **Knowledge Base Management**
- **IM Integration**
- **Proxy Settings**
- **User Agreement**

### Model Configuration

The **Model Configuration** page is used to view the overall current model setup.

![19|settings model](fig/p19-settings-model.png)

On the current page:

1. The **Online model** section shows the current account **UOS AI Trial Account**.
2. **Delete** and **Add** are shown in the upper-right corner.
3. The **Local model** section shows **Embedding Plugins** and **DeepSeek-R1-1.5B**.
4. The **Private deployment model** section currently shows **None**.

### Online Model

Click **Online model** on the left side to move to the online model section, where the current account can be viewed and managed through **Add** or **Delete**.

![image-20260508175205712](fig/image-20260508175205712.png)

On the current page:

1. **Add** is used to add a new online model account.
2. **Delete** is used to enter the account management state.
3. Clicking an existing online model item opens the corresponding account or model details.

If **Add** is clicked, the online model adding flow opens. In the current verified flow, the user needs to:

1. Enter an account name.
2. Select a service provider.
3. Enter the **API Key**.
4. Select the models to enable.
5. Click **Confirm** to save the configuration.

### Local Model

Click **Local model** on the left side to move to the local model section and check the installation status of local models.

![20|settings local](fig/p20-settings-local.png)

On the current page:

1. **Embedding Plugins** is already shown as installed.
2. **DeepSeek-R1-1.5B** shows an **Install** button on the right side.
3. When a local model is in a download or install process, the button can switch to **Cancel**.
4. Installed items show **Uninstall** on the right side.

In the current version, **Embedding Plugins** is a prerequisite component for the knowledge base and some local capabilities.

### Private Deployment Model

Click **Private deployment model** on the left side to move to the private deployment section.

![21|settings private](fig/p21-settings-private.png)

If **Add** is clicked, the private deployment model adding flow opens.

![image-20260508175302132](fig/image-20260508175302132.png)

In the current verified flow, the user needs to:

1. Enter an account name.
2. Select **Private deployment** as the service provider.
3. Enter the **API Key**.
4. Enter the request address.
5. Add the model ID and model name in the custom section.
6. Click **Test** to verify model availability.
7. Click **Confirm** to save the model.

### MCP Server

The **MCP Server** page currently contains three main sections:

1. **Mcp Environment**
2. **UOS AI FollowAlong** preview
3. **Skill Management**

![22|settings mcp](fig/p22-settings-mcp.png)

On the current page:

1. The **Mcp Environment** section shows the current state of `UOS AI Agent`.
2. The **UOS AI FollowAlong** section displays a toolbar preview.
3. The **Skill Management** section shows the current preset skill list.
4. The right side includes **Add Skill**.

### UOS AI FollowAlong

The **UOS AI FollowAlong** page is used to manage the FollowAlong toolbar behavior.

![23|settings follow](fig/p23-settings-follow.png)

On this page:

1. The main switch controls whether the FollowAlong toolbar appears automatically when text is selected.
2. The skill list below is used to manage the display order of toolbar skills.
3. The current visible skills include entries such as **Search**, **Explain**, **Summary**, and **Translate**.
4. The right side provides **Add Skill**.

### Knowledge Base Management

The **Knowledge Base Management** page is used to manage files for **AI Knowledge Base**.

![24|settings knowledge base](fig/p24-settings-kb.png)

On this page:

1. **Add** in the upper-right corner is used to import knowledge base files.
2. **Delete** in the upper-right corner is used to remove imported entries.
3. The page displays the current capacity, for example `5.2KB/1024M`.
4. Imported files are listed with their names and sizes.

### IM Integration

The **IM Integration** page is used to manage third-party message forwarding.

![25|settings im](fig/p25-settings-im.png)

![image-20260508175349382](fig/image-20260508175349382.png)

On the current page:

1. The top section includes the **Enable Message Forwarding Service** switch.
2. The currently visible IM entries include **Lark**, **DingTalk**, and **QQ**.

After the corresponding IM switch is enabled, the IM configuration dialog can be opened for that platform.

### Proxy Settings

The **Proxy Settings** page is used to configure the proxy environment required for model access.

![26|settings proxy](fig/p26-settings-proxy.png)

In the current version, proxy setup bridges to system settings through **Go to settings**.

### User Agreement

The **User Agreement** page is used to view the current UOS AI agreement status.

![27|settings agreement](fig/p27-settings-agreement.png)

## Plugin

### UOS AI FollowAlong

**Wake-up Method**

After text is selected in the system interface, if FollowAlong is enabled, the system automatically shows the FollowAlong toolbar. If it is disabled, the toolbar no longer appears automatically after text selection, and it can be enabled again from the settings page.
