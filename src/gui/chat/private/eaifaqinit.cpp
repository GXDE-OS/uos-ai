#include "eaifaqinit.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <random>

EAiFAQInit::EAiFAQInit(QObject *parent) : QObject(parent)
{
}

QJsonArray EAiFAQInit::createTranslationFAQArray() const
{
    QJsonArray faqArray;

    // 问题1
    QJsonObject question1;
    question1["iconName"] = "17-question-ai";
    question1["Question"] = tr("Translate the following text into English for me.");
    faqArray.append(question1);

    // 问题2
    QJsonObject question2;
    question2["iconName"] = "17-question-ai";
    question2["Question"] = tr("Translate the following document into Chinese.");
    faqArray.append(question2);

    // 问题3
    QJsonObject question3;
    question3["iconName"] = "17-question-ai";
    question3["Question"] = tr("What does the word “Agent” mean in the AI industry?");
    faqArray.append(question3);

    // 问题4
    QJsonObject question4;
    question4["iconName"] = "17-question-ai";
    question4["Question"] = tr("Please translate the following content into Chinese. Requirements: Accurate in meaning, formal and professional in language.");
    faqArray.append(question4);

    // 问题5
    QJsonObject question5;
    question5["iconName"] = "17-question-ai";
    question5["Question"] = tr("What are some colloquial ways to address a friend in English?");
    faqArray.append(question5);

    // 问题6
    QJsonObject question6;
    question6["iconName"] = "17-question-ai";
    question6["Question"] = tr("Translate the following classical Chinese text into modern Chinese.");
    faqArray.append(question6);

    return faqArray;
}

QJsonArray EAiFAQInit::createTextProcessingFunctionArray() const
{
    QJsonArray faqArray;

    QJsonObject item1;
    item1["iconName"] = "summarize";
    item1["Function"] = "Summarize";
    item1["Name"] = tr("Summarize");
    faqArray.append(item1);

    QJsonObject item2;
    item2["iconName"] = "proofread";
    item2["Function"] = "Proofread";
    item2["Name"] = tr("Proofread");
    faqArray.append(item2);

    QJsonObject item3;
    item3["iconName"] = "explain";
    item3["Function"] = "Explain";
    item3["Name"] = tr("Explain");
    faqArray.append(item3);

    QJsonObject item4;
    item4["iconName"] = "expand";
    item4["Function"] = "Expand";
    item4["Name"] = tr("Expand");
    faqArray.append(item4);

    QJsonObject item5;
    item5["iconName"] = "continue";
    item5["Function"] = "Continue";
    item5["Name"] = tr("Continue");
    faqArray.append(item5);

    QJsonObject item6;
    item6["iconName"] = "polish";
    item6["Function"] = "Polish";
    item6["Name"] = tr("Polish");
    faqArray.append(item6);

    return faqArray;
}

QJsonArray EAiFAQInit::createTextProcessingFAQArray() const
{
    QJsonArray faqArray;

    // Summarize 类别
    QJsonObject question1;
    question1["iconName"] = "17-question-ai";
    question1["Function"] = "Summarize";
    question1["Question"] = tr("Please help me write a work summary based on this document");
    faqArray.append(question1);

    QJsonObject question2;
    question2["iconName"] = "17-question-ai";
    question2["Function"] = "Summarize";
    question2["Question"] = tr("I need a work summary for Project A, please help me organize the key points");
    faqArray.append(question2);

    QJsonObject question3;
    question3["iconName"] = "17-question-ai";
    question3["Function"] = "Summarize";
    question3["Question"] = tr("Please generate a concise work summary based on my input text");
    faqArray.append(question3);

    QJsonObject question4;
    question4["iconName"] = "17-question-ai";
    question4["Function"] = "Summarize";
    question4["Question"] = tr("Analyze this report and extract key points for the work summary");
    faqArray.append(question4);

    QJsonObject question5;
    question5["iconName"] = "17-question-ai";
    question5["Function"] = "Summarize";
    question5["Question"] = tr("I need a work summary including quarterly data, please refer to the attachment");
    faqArray.append(question5);

    QJsonObject question6;
    question6["iconName"] = "17-question-ai";
    question6["Function"] = "Summarize";
    question6["Question"] = tr("Help me write a work summary highlighting personal contributions");
    faqArray.append(question6);

    QJsonObject question7;
    question7["iconName"] = "17-question-ai";
    question7["Function"] = "Summarize";
    question7["Question"] = tr("Generate a work summary from meeting notes, focusing on action items");
    faqArray.append(question7);

    QJsonObject question8;
    question8["iconName"] = "17-question-ai";
    question8["Function"] = "Summarize";
    question8["Question"] = tr("I need a bilingual work summary in Chinese and English, please refer to the provided materials");
    faqArray.append(question8);

    QJsonObject question9;
    question9["iconName"] = "17-question-ai";
    question9["Function"] = "Summarize";
    question9["Question"] = tr("Help me organize last month's work content and generate a summary");
    faqArray.append(question9);

    QJsonObject question10;
    question10["iconName"] = "17-question-ai";
    question10["Function"] = "Summarize";
    question10["Question"] = tr("Write a work summary based on these task lists, emphasizing completion status");
    faqArray.append(question10);

    // Proofread 类别
    QJsonObject question11;
    question11["iconName"] = "17-question-ai";
    question11["Function"] = "Proofread";
    question11["Question"] = tr("Check for grammatical errors in this passage");
    faqArray.append(question11);

    QJsonObject question12;
    question12["iconName"] = "17-question-ai";
    question12["Function"] = "Proofread";
    question12["Question"] = tr("Help me find spelling mistakes in this article");
    faqArray.append(question12);

    QJsonObject question13;
    question13["iconName"] = "17-question-ai";
    question13["Function"] = "Proofread";
    question13["Question"] = tr("Correct the inappropriate word choices in this dialogue");
    faqArray.append(question13);

    QJsonObject question14;
    question14["iconName"] = "17-question-ai";
    question14["Function"] = "Proofread";
    question14["Question"] = tr("Fix the punctuation issues in this document");
    faqArray.append(question14);

    QJsonObject question15;
    question15["iconName"] = "17-question-ai";
    question15["Function"] = "Proofread";
    question15["Question"] = tr("Revise the unclear expressions in this text");
    faqArray.append(question15);

    QJsonObject question16;
    question16["iconName"] = "17-question-ai";
    question16["Function"] = "Proofread";
    question16["Question"] = tr("Edit the language errors in this email");
    faqArray.append(question16);

    QJsonObject question17;
    question17["iconName"] = "17-question-ai";
    question17["Function"] = "Proofread";
    question17["Question"] = tr("Check if this text follows standard English conventions");
    faqArray.append(question17);

    QJsonObject question18;
    question18["iconName"] = "17-question-ai";
    question18["Function"] = "Proofread";
    question18["Question"] = tr("Correct the logical flaws in this report");
    faqArray.append(question18);

    QJsonObject question19;
    question19["iconName"] = "17-question-ai";
    question19["Function"] = "Proofread";
    question19["Question"] = tr("Identify language errors in this code comment");
    faqArray.append(question19);

    QJsonObject question20;
    question20["iconName"] = "17-question-ai";
    question20["Function"] = "Proofread";
    question20["Question"] = tr("Fix the ambiguous statements in this contract");
    faqArray.append(question20);

    // Explain 类别
    QJsonObject question21;
    question21["iconName"] = "17-question-ai";
    question21["Function"] = "Explain";
    question21["Question"] = tr("Explain the basic principles of quantum computing");
    faqArray.append(question21);

    QJsonObject question22;
    question22["iconName"] = "17-question-ai";
    question22["Function"] = "Explain";
    question22["Question"] = tr("Describe photosynthesis in simple terms");
    faqArray.append(question22);

    QJsonObject question23;
    question23["iconName"] = "17-question-ai";
    question23["Function"] = "Explain";
    question23["Question"] = tr("Detail how blockchain technology works");
    faqArray.append(question23);

    QJsonObject question24;
    question24["iconName"] = "17-question-ai";
    question24["Function"] = "Explain";
    question24["Question"] = tr("Explain relativity from a professional perspective");
    faqArray.append(question24);

    QJsonObject question25;
    question25["iconName"] = "17-question-ai";
    question25["Function"] = "Explain";
    question25["Question"] = tr("Use metaphors to describe what artificial intelligence is");
    faqArray.append(question25);

    QJsonObject question26;
    question26["iconName"] = "17-question-ai";
    question26["Function"] = "Explain";
    question26["Question"] = tr("Describe climate change in layman's terms");
    faqArray.append(question26);

    QJsonObject question27;
    question27["iconName"] = "17-question-ai";
    question27["Function"] = "Explain";
    question27["Question"] = tr("Explain the structure of the solar system to elementary students");
    faqArray.append(question27);

    // Expand 类别
    QJsonObject question28;
    question28["iconName"] = "17-question-ai";
    question28["Function"] = "Expand";
    question28["Question"] = tr("Expand this text to 300 words, focusing on scene details and character emotions");
    faqArray.append(question28);

    QJsonObject question29;
    question29["iconName"] = "17-question-ai";
    question29["Function"] = "Expand";
    question29["Question"] = tr("Rewrite this paragraph into 500 words in an academic style, adding data support and citations");
    faqArray.append(question29);

    QJsonObject question30;
    question30["iconName"] = "17-question-ai";
    question30["Function"] = "Expand";
    question30["Question"] = tr("Expand this passage with more metaphors and rhetorical devices to make it more vivid");
    faqArray.append(question30);

    QJsonObject question31;
    question31["iconName"] = "17-question-ai";
    question31["Function"] = "Expand";
    question31["Question"] = tr("Please extend this short article to 1000 words, supplementing with specific cases and practical steps");
    faqArray.append(question31);

    QJsonObject question32;
    question32["iconName"] = "17-question-ai";
    question32["Function"] = "Expand";
    question32["Question"] = tr("Expand this dialogue by adding characters' psychological activities and dialogue details");
    faqArray.append(question32);

    QJsonObject question33;
    question33["iconName"] = "17-question-ai";
    question33["Function"] = "Expand";
    question33["Question"] = tr("Rewrite the content of this document into a report format, including abstract, body, and conclusion");
    faqArray.append(question33);

    QJsonObject question34;
    question34["iconName"] = "17-question-ai";
    question34["Function"] = "Expand";
    question34["Question"] = tr("Expand this technical description to 800 words, adding more technical parameters and diagram explanations");
    faqArray.append(question34);

    QJsonObject question35;
    question35["iconName"] = "17-question-ai";
    question35["Function"] = "Expand";
    question35["Question"] = tr("Rewrite this email to be more formal and professional, supplementing relevant background information");
    faqArray.append(question35);

    QJsonObject question36;
    question36["iconName"] = "17-question-ai";
    question36["Function"] = "Expand";
    question36["Question"] = tr("Expand this story to 1500 words, adding more plot twists and character backgrounds");
    faqArray.append(question36);

    QJsonObject question37;
    question37["iconName"] = "17-question-ai";
    question37["Function"] = "Expand";
    question37["Question"] = tr("Please expand this summary, highlighting key achievements and future plans, word count unlimited");
    faqArray.append(question37);

    // Continue 类别
    QJsonObject question38;
    question38["iconName"] = "17-question-ai";
    question38["Function"] = "Continue";
    question38["Question"] = tr("Please continue writing this short passage about future technology, focusing on how AI changes daily life, around 300 words, in a popular science style");
    faqArray.append(question38);

    QJsonObject question39;
    question39["iconName"] = "17-question-ai";
    question39["Function"] = "Continue";
    question39["Question"] = tr("Help me expand the last paragraph of this business plan, emphasizing market potential and competitive advantages, no word limit, formal style");
    faqArray.append(question39);

    QJsonObject question40;
    question40["iconName"] = "17-question-ai";
    question40["Function"] = "Continue";
    question40["Question"] = tr("Continue the ending of this fairy tale, warm and touching, the protagonist achieves their dream, about 200 words");
    faqArray.append(question40);

    QJsonObject question41;
    question41["iconName"] = "17-question-ai";
    question41["Function"] = "Continue";
    question41["Question"] = tr("Please write the second half of this email, tone friendly and professional, focusing on collaboration details, within 150 words");
    faqArray.append(question41);

    QJsonObject question42;
    question42["iconName"] = "17-question-ai";
    question42["Function"] = "Continue";
    question42["Question"] = tr("Expand the next paragraph of this travelogue, describing the scenery in the valley, vivid and imaginative, around 200 words");
    faqArray.append(question42);

    QJsonObject question43;
    question43["iconName"] = "17-question-ai";
    question43["Function"] = "Continue";
    question43["Question"] = tr("Continue the next chapter of this mystery novel, with a plot twist, maintaining suspense, no word limit");
    faqArray.append(question43);

    QJsonObject question44;
    question44["iconName"] = "17-question-ai";
    question44["Function"] = "Continue";
    question44["Question"] = tr("Help me write the conclusion of this speech, inspiring and emphasizing teamwork, about 100 words");
    faqArray.append(question44);

    QJsonObject question45;
    question45["iconName"] = "17-question-ai";
    question45["Function"] = "Continue";
    question45["Question"] = tr("Expand the performance description section of this product manual, detailed and accurate, around 300 words");
    faqArray.append(question45);

    QJsonObject question46;
    question46["iconName"] = "17-question-ai";
    question46["Function"] = "Continue";
    question46["Question"] = tr("Continue the beginning of this love story, set in rainy Paris, romantic style, 150 words");
    faqArray.append(question46);

    QJsonObject question47;
    question47["iconName"] = "17-question-ai";
    question47["Function"] = "Continue";
    question47["Question"] = tr("Please write the discussion section of this paper, focusing on the limitations of the experimental result, academic style, no word limit");
    faqArray.append(question47);

    // Polish 类别
    QJsonObject question48;
    question48["iconName"] = "17-question-ai";
    question48["Function"] = "Polish";
    question48["Question"] = tr("Please polish this text to make it more formal and professional for a business email");
    faqArray.append(question48);

    QJsonObject question49;
    question49["iconName"] = "17-question-ai";
    question49["Function"] = "Polish";
    question49["Question"] = tr("Rewrite this passage to be more lively and engaging for social media posts");
    faqArray.append(question49);

    QJsonObject question50;
    question50["iconName"] = "17-question-ai";
    question50["Function"] = "Polish";
    question50["Question"] = tr("I need to refine the abstract of this academic paper to be more concise and clear within 200 words");
    faqArray.append(question50);

    QJsonObject question51;
    question51["iconName"] = "17-question-ai";
    question51["Function"] = "Polish";
    question51["Question"] = tr("Polish this dialogue to make it more natural and fluent for spoken communication");
    faqArray.append(question51);

    QJsonObject question52;
    question52["iconName"] = "17-question-ai";
    question52["Function"] = "Polish";
    question52["Question"] = tr("Enhance this product description to highlight its premium and innovative features for potential customers");
    faqArray.append(question52);

    QJsonObject question53;
    question53["iconName"] = "17-question-ai";
    question53["Function"] = "Polish";
    question53["Question"] = tr("Help me polish this cover letter to make it more persuasive and personalized");
    faqArray.append(question53);

    QJsonObject question54;
    question54["iconName"] = "17-question-ai";
    question54["Function"] = "Polish";
    question54["Question"] = tr("Rewrite this blog post to be more humorous and appealing to young readers");
    faqArray.append(question54);

    QJsonObject question55;
    question55["iconName"] = "17-question-ai";
    question55["Function"] = "Polish";
    question55["Question"] = tr("Refine the opening section of this report to make it more captivating");
    faqArray.append(question55);

    QJsonObject question56;
    question56["iconName"] = "17-question-ai";
    question56["Function"] = "Polish";
    question56["Question"] = tr("I need to polish this technical document to make it more accessible for non-experts");
    faqArray.append(question56);

    QJsonObject question57;
    question57["iconName"] = "17-question-ai";
    question57["Function"] = "Polish";
    question57["Question"] = tr("Please help me rewrite this apology letter to sound more sincere and appropriate");
    faqArray.append(question57);

    return faqArray;
}

QJsonArray EAiFAQInit::createWritingFunctionArray() const
{
    QJsonArray faqArray;

    QJsonObject item1;
    item1["iconName"] = "articles";
    item1["Function"] = "Articles";
    item1["Name"] = tr("Articles");
    faqArray.append(item1);

    QJsonObject item2;
    item2["iconName"] = "speeches";
    item2["Function"] = "Speeches";
    item2["Name"] = tr("Speeches");
    faqArray.append(item2);

    QJsonObject item3;
    item3["iconName"] = "outlines";
    item3["Function"] = "Outlines";
    item3["Name"] = tr("Outlines");
    faqArray.append(item3);

    QJsonObject item4;
    item4["iconName"] = "notifications";
    item4["Function"] = "Notifications";
    item4["Name"] = tr("Notifications");
    faqArray.append(item4);

    QJsonObject item5;
    item5["iconName"] = "posts";
    item5["Function"] = "Posts";
    item5["Name"] = tr("Posts");
    faqArray.append(item5);

    QJsonObject item6;
    item6["iconName"] = "work-report";
    item6["Function"] = "Work Report";
    item6["Name"] = tr("Work Report");
    faqArray.append(item6);

    QJsonObject item7;
    item7["iconName"] = "research-report";
    item7["Function"] = "Research Report";
    item7["Name"] = tr("Research Report");
    faqArray.append(item7);

    return faqArray;
}

QJsonArray EAiFAQInit::createWritingFAQArray() const
{
    QJsonArray faqArray;

    // Articles 类别
    QJsonObject question1;
    question1["iconName"] = "17-question-ai";
    question1["Function"] = "Research Report";
    question1["Question"] = tr("Write a report on the development history of domestic operating systems, focusing on rapid development in the past 5 years, using only my local content as reference");
    faqArray.append(question1);

    QJsonObject question2;
    question2["iconName"] = "17-question-ai";
    question2["Function"] = "Research Report";
    question2["Question"] = tr("Write a report on the development history of domestic operating systems, using my local materials and internet content as sources");
    faqArray.append(question2);

    QJsonObject question3;
    question3["iconName"] = "17-question-ai";
    question3["Function"] = "Articles";
    question3["Question"] = tr("Summarize the core idea of this passage");
    faqArray.append(question3);

    QJsonObject question4;
    question4["iconName"] = "17-question-ai";
    question4["Function"] = "Articles";
    question4["Question"] = tr("Use concise language to summarize this article");
    faqArray.append(question4);

    QJsonObject question5;
    question5["iconName"] = "17-question-ai";
    question5["Function"] = "Articles";
    question5["Question"] = tr("I need a summary of this report, no more than 200 words");
    faqArray.append(question5);

    QJsonObject question6;
    question6["iconName"] = "17-question-ai";
    question6["Function"] = "Articles";
    question6["Question"] = tr("Extract the main data and conclusions from this document");
    faqArray.append(question6);

    QJsonObject question7;
    question7["iconName"] = "17-question-ai";
    question7["Function"] = "Articles";
    question7["Question"] = tr("Summarize the main sections and points of this lengthy content");
    faqArray.append(question7);

    QJsonObject question8;
    question8["iconName"] = "17-question-ai";
    question8["Function"] = "Articles";
    question8["Question"] = tr("Help me summarize the research methods and findings of this paper");
    faqArray.append(question8);

    QJsonObject question9;
    question9["iconName"] = "17-question-ai";
    question9["Function"] = "Articles";
    question9["Question"] = tr("Please summarize the key points of this article in a table format");
    faqArray.append(question9);

    QJsonObject question10;
    question10["iconName"] = "17-question-ai";
    question10["Function"] = "Articles";
    question10["Question"] = tr("Summarize the operational steps in this file using bullet points");
    faqArray.append(question10);

    QJsonObject question11;
    question11["iconName"] = "17-question-ai";
    question11["Function"] = "Articles";
    question11["Question"] = tr("Display the key clauses of this contract in bullet list format");
    faqArray.append(question11);

    // Speeches 类别
    QJsonObject question12;
    question12["iconName"] = "17-question-ai";
    question12["Function"] = "Speeches";
    question12["Question"] = tr("Create an outline for a report on AI trends, covering technology, ethics, and applications");
    faqArray.append(question12);

    QJsonObject question13;
    question13["iconName"] = "17-question-ai";
    question13["Function"] = "Speeches";
    question13["Question"] = tr("Organize the uploaded paper into a structured research framework outline");
    faqArray.append(question13);

    QJsonObject question14;
    question14["iconName"] = "17-question-ai";
    question14["Function"] = "Speeches";
    question14["Question"] = tr("I need a business plan outline including market analysis, product description, and financial planning");
    faqArray.append(question14);

    QJsonObject question15;
    question15["iconName"] = "17-question-ai";
    question15["Function"] = "Speeches";
    question15["Question"] = tr("Convert this speech into a PowerPoint outline with bullet points for each section");
    faqArray.append(question15);

    QJsonObject question16;
    question16["iconName"] = "17-question-ai";
    question16["Function"] = "Speeches";
    question16["Question"] = tr("Write a book summary outline covering key points and my personal reflections");
    faqArray.append(question16);

    QJsonObject question17;
    question17["iconName"] = "17-question-ai";
    question17["Function"] = "Speeches";
    question17["Question"] = tr("Summarize the meeting notes file into a project execution plan outline");
    faqArray.append(question17);

    QJsonObject question18;
    question18["iconName"] = "17-question-ai";
    question18["Function"] = "Speeches";
    question18["Question"] = tr("I need a thesis chapter outline including introduction, literature review, and methodology");
    faqArray.append(question18);

    QJsonObject question19;
    question19["iconName"] = "17-question-ai";
    question19["Function"] = "Speeches";
    question19["Question"] = tr("Create a product requirements document (PRD) outline with feature descriptions and user flows");
    faqArray.append(question19);

    QJsonObject question20;
    question20["iconName"] = "17-question-ai";
    question20["Function"] = "Speeches";
    question20["Question"] = tr("Extract key data from this industry report to make a summary outline");
    faqArray.append(question20);

    QJsonObject question21;
    question21["iconName"] = "17-question-ai";
    question21["Function"] = "Speeches";
    question21["Question"] = tr("Write an event planning outline including schedule, team roles, and budget");
    faqArray.append(question21);

    // Outlines 类别
    QJsonObject question22;
    question22["iconName"] = "17-question-ai";
    question22["Function"] = "Outlines";
    question22["Question"] = tr("Generate a three-level outline from this document");
    faqArray.append(question22);

    QJsonObject question23;
    question23["iconName"] = "17-question-ai";
    question23["Function"] = "Outlines";
    question23["Question"] = tr("Extract core framework from meeting notes");
    faqArray.append(question23);

    QJsonObject question24;
    question24["iconName"] = "17-question-ai";
    question24["Function"] = "Outlines";
    question24["Question"] = tr("Organize research report structure");
    faqArray.append(question24);

    QJsonObject question25;
    question25["iconName"] = "17-question-ai";
    question25["Function"] = "Outlines";
    question25["Question"] = tr("Create product feature overview");
    faqArray.append(question25);

    QJsonObject question26;
    question26["iconName"] = "17-question-ai";
    question26["Function"] = "Outlines";
    question26["Question"] = tr("Design training course syllabus");
    faqArray.append(question26);

    QJsonObject question27;
    question27["iconName"] = "17-question-ai";
    question27["Function"] = "Outlines";
    question27["Question"] = tr("Analyze literature review framework");
    faqArray.append(question27);

    QJsonObject question28;
    question28["iconName"] = "17-question-ai";
    question28["Function"] = "Outlines";
    question28["Question"] = tr("Generate business plan table of contents");
    faqArray.append(question28);

    QJsonObject question29;
    question29["iconName"] = "17-question-ai";
    question29["Function"] = "Outlines";
    question29["Question"] = tr("Write event planning flow points");
    faqArray.append(question29);

    // Notifications 类别
    QJsonObject question30;
    question30["iconName"] = "17-question-ai";
    question30["Function"] = "Notifications";
    question30["Question"] = tr("Help me write a meeting notice for tomorrow at 3 PM in Conference Room A about project progress reporting");
    faqArray.append(question30);

    QJsonObject question31;
    question31["iconName"] = "17-question-ai";
    question31["Function"] = "Notifications";
    question31["Question"] = tr("Create a concise notice based on this document highlighting the deadline and submission method");
    faqArray.append(question31);

    QJsonObject question32;
    question32["iconName"] = "17-question-ai";
    question32["Function"] = "Notifications";
    question32["Question"] = tr("Draft a holiday notice for Spring Festival from February 10 to 17 with work resuming on February 18");
    faqArray.append(question32);

    QJsonObject question33;
    question33["iconName"] = "17-question-ai";
    question33["Function"] = "Notifications";
    question33["Question"] = tr("I need an activity notice for staff training all day next Wednesday at the company auditorium");
    faqArray.append(question33);

    QJsonObject question34;
    question34["iconName"] = "17-question-ai";
    question34["Function"] = "Notifications";
    question34["Question"] = tr("Format the provided text materials into a formal notice with standard formatting and professional tone");
    faqArray.append(question34);

    QJsonObject question35;
    question35["iconName"] = "17-question-ai";
    question35["Function"] = "Notifications";
    question35["Question"] = tr("Write a power outage notice for this Saturday from 9 AM to 5 PM affecting the entire office building");
    faqArray.append(question35);

    QJsonObject question36;
    question36["iconName"] = "17-question-ai";
    question36["Function"] = "Notifications";
    question36["Question"] = tr("Generate a meeting change notice shifting the original time to Friday at 10 AM keeping other details same");
    faqArray.append(question36);

    QJsonObject question37;
    question37["iconName"] = "17-question-ai";
    question37["Function"] = "Notifications";
    question37["Question"] = tr("Prepare a brief notice based on the attached meeting minutes for absent attendees");
    faqArray.append(question37);

    QJsonObject question38;
    question38["iconName"] = "17-question-ai";
    question38["Function"] = "Notifications";
    question38["Question"] = tr("Draft a system upgrade notice scheduled tonight from midnight to 3 AM with system downtime");
    faqArray.append(question38);

    QJsonObject question39;
    question39["iconName"] = "17-question-ai";
    question39["Function"] = "Notifications";
    question39["Question"] = tr("Write a concise holiday duty arrangement notice listing on-duty staff and contact information");
    faqArray.append(question39);

    // Posts 类别
    QJsonObject question40;
    question40["iconName"] = "17-question-ai";
    question40["Function"] = "Posts";
    question40["Question"] = tr("Help me write a public health article about healthy eating in simple language for general readers");
    faqArray.append(question40);

    QJsonObject question41;
    question41["iconName"] = "17-question-ai";
    question41["Function"] = "Posts";
    question41["Question"] = tr("Create a fun and engaging tech trend post based on this document");
    faqArray.append(question41);

    QJsonObject question42;
    question42["iconName"] = "17-question-ai";
    question42["Function"] = "Posts";
    question42["Question"] = tr("Draft a holiday promotion post highlighting discounts and event schedule");
    faqArray.append(question42);

    QJsonObject question43;
    question43["iconName"] = "17-question-ai";
    question43["Function"] = "Posts";
    question43["Question"] = tr("I need a product launch article focusing on features and customer reviews");
    faqArray.append(question43);

    QJsonObject question44;
    question44["iconName"] = "17-question-ai";
    question44["Function"] = "Posts";
    question44["Question"] = tr("Turn this interview transcript into a casual personality profile post");
    faqArray.append(question44);

    QJsonObject question45;
    question45["iconName"] = "17-question-ai";
    question45["Function"] = "Posts";
    question45["Question"] = tr("Write a travel guide recommending three popular spots with practical tips");
    faqArray.append(question45);

    QJsonObject question46;
    question46["iconName"] = "17-question-ai";
    question46["Function"] = "Posts";
    question46["Question"] = tr("Generate an anniversary post reviewing company history and thanking customers");
    faqArray.append(question46);

    QJsonObject question47;
    question47["iconName"] = "17-question-ai";
    question47["Function"] = "Posts";
    question47["Question"] = tr("Analyze this industry report with visualized data for an insightful post");
    faqArray.append(question47);

    QJsonObject question48;
    question48["iconName"] = "17-question-ai";
    question48["Function"] = "Posts";
    question48["Question"] = tr("Draft a charity campaign post encouraging participation with clear instructions");
    faqArray.append(question48);

    QJsonObject question49;
    question49["iconName"] = "17-question-ai";
    question49["Function"] = "Posts";
    question49["Question"] = tr("Write a career skills article with five actionable tips and real examples");
    faqArray.append(question49);

    // Work Report 类别
    QJsonObject question50;
    question50["iconName"] = "17-question-ai";
    question50["Function"] = "Work Report";
    question50["Question"] = tr("Help me write a weekly work summary focusing on project progress and team collaboration");
    faqArray.append(question50);

    QJsonObject question51;
    question51["iconName"] = "17-question-ai";
    question51["Function"] = "Work Report";
    question51["Question"] = tr("Generate a concise work summary based on the meeting notes file I uploaded");
    faqArray.append(question51);

    QJsonObject question52;
    question52["iconName"] = "17-question-ai";
    question52["Function"] = "Work Report";
    question52["Question"] = tr("I need a quarterly work summary including data analysis and personal growth reflection");
    faqArray.append(question52);

    QJsonObject question53;
    question53["iconName"] = "17-question-ai";
    question53["Function"] = "Work Report";
    question53["Question"] = tr("Create a work summary based on the following text [paste text]");
    faqArray.append(question53);

    QJsonObject question54;
    question54["iconName"] = "17-question-ai";
    question54["Function"] = "Work Report";
    question54["Question"] = tr("Write a work summary highlighting this month's KPI achievements and challenges encountered");
    faqArray.append(question54);

    QJsonObject question55;
    question55["iconName"] = "17-question-ai";
    question55["Function"] = "Work Report";
    question55["Question"] = tr("Help me summarize last week's work categorized by projects");
    faqArray.append(question55);

    QJsonObject question56;
    question56["iconName"] = "17-question-ai";
    question56["Function"] = "Work Report";
    question56["Question"] = tr("Generate a work summary from email content emphasizing pending tasks and completions");
    faqArray.append(question56);

    QJsonObject question57;
    question57["iconName"] = "17-question-ai";
    question57["Function"] = "Work Report";
    question57["Question"] = tr("I need an annual summary including work highlights and improvement plans");
    faqArray.append(question57);

    QJsonObject question58;
    question58["iconName"] = "17-question-ai";
    question58["Function"] = "Work Report";
    question58["Question"] = tr("Generate a monthly work report from the data I uploaded");
    faqArray.append(question58);

    QJsonObject question59;
    question59["iconName"] = "17-question-ai";
    question59["Function"] = "Work Report";
    question59["Question"] = tr("Help me organize scattered work notes into a structured work summary");
    faqArray.append(question59);

    // Research Report 类别
    QJsonObject question60;
    question60["iconName"] = "17-question-ai";
    question60["Function"] = "Research Report";
    question60["Question"] = tr("Analyze the uploaded data and generate a market trend report");
    faqArray.append(question60);

    QJsonObject question61;
    question61["iconName"] = "17-question-ai";
    question61["Function"] = "Research Report";
    question61["Question"] = tr("Extract key information from the text and compile an industry analysis report");
    faqArray.append(question61);

    QJsonObject question62;
    question62["iconName"] = "17-question-ai";
    question62["Function"] = "Research Report";
    question62["Question"] = tr("Combine the contents of the file to write a competitor research report");
    faqArray.append(question62);

    QJsonObject question63;
    question63["iconName"] = "17-question-ai";
    question63["Function"] = "Research Report";
    question63["Question"] = tr("Generate a user needs research summary based on the provided interview records");
    faqArray.append(question63);

    QJsonObject question64;
    question64["iconName"] = "17-question-ai";
    question64["Function"] = "Research Report";
    question64["Question"] = tr("Analyze these sales data and produce a quarterly market performance report");
    faqArray.append(question64);

    QJsonObject question65;
    question65["iconName"] = "17-question-ai";
    question65["Function"] = "Research Report";
    question65["Question"] = tr("Write a technical research report based on the given product description");
    faqArray.append(question65);

    QJsonObject question66;
    question66["iconName"] = "17-question-ai";
    question66["Function"] = "Research Report";
    question66["Question"] = tr("Organize these user feedback and generate a product improvement suggestion report");
    faqArray.append(question66);

    QJsonObject question67;
    question67["iconName"] = "17-question-ai";
    question67["Function"] = "Research Report";
    question67["Question"] = tr("Please write a market opportunity analysis report based on the provided file");
    faqArray.append(question67);

    QJsonObject question68;
    question68["iconName"] = "17-question-ai";
    question68["Function"] = "Articles";
    question1["Question"] = tr("Please summarize the key points of this document");
    faqArray.append(question68);

    QJsonObject question69;
    question69["iconName"] = "17-question-ai";
    question69["Function"] = "Articles";
    question69["Question"] = tr("List the main ideas of this file for me");
    faqArray.append(question69);

    return faqArray;
}

QByteArray EAiFAQInit::createWritingFunctionTemplate() const
{
    QJsonObject root;

    QJsonObject articles;
    articles["Template"] = tr("Help me write an essay on the topic of [%1] with [clear structure and rich content].");
    articles["Default"] = tr("Artificial Intelligence");
    root["Articles"] = articles;

    QJsonObject speeches;
    speeches["Template"] = tr("Help me write a speech on the topic of [%1] for [Company Leaders], requiring [clear structure and vivid language].");
    speeches["Default"] = tr("Artificial Intelligence");
    root["Speeches"] = speeches;

    QJsonObject outlines;
    outlines["Template"] = tr("Help me write an outline on the topic of [%1], which will be used for [PPT production].");
    outlines["Default"] = tr("Artificial Intelligence");
    root["Outlines"] = outlines;

    QJsonObject notifications;
    notifications["Template"] = tr("Help me write a notice about [%1], the receiver is [All Employees] and the sender is [Administration Department].");
    notifications["Default"] = tr("National Day Holiday");
    root["Notifications"] = notifications;

    QJsonObject posts;
    posts["Template"] = tr("Help me write a public tweet on the topic of [%1], requiring [clear structure] and [relaxed] tone.");
    posts["Default"] = tr("Artificial Intelligence");
    root["Posts"] = posts;

    QJsonObject workReport;
    workReport["Template"] = tr("Help me write a summary of my recent work, including [%1] and [results], requiring a [formal] tone.");
    workReport["Default"] = tr("work content");
    root["Work Report"] = workReport;

    QJsonObject researchReport;
    researchReport["Template"] = tr("Help me write a research report on [%1], at least it needs to include [status description, problem analysis, countermeasures and suggestions, research conclusions].");
    researchReport["Default"] = tr("Artificial Intelligence");
    root["Research Report"] = researchReport;

    QJsonDocument doc(root);
    return doc.toJson();
}

QJsonArray EAiFAQInit::assignRandomIcons(QJsonArray jsonArray, const QString iconName) const
{
    // Create a list of available icon numbers
    QList<int> availableIcons;
    int iconSize = jsonArray.size();

    if (iconName == "general") {
        iconSize = 12;
    } else if (iconName == "operations") {
        iconSize = 18;
    }

    for (int i = 1; i <= iconSize; ++i) {
        availableIcons.append(i);
    }

    // Shuffle the available icons
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(availableIcons.begin(), availableIcons.end(), g);

    // Assign icons to each object in the array
    for (int i = 0; i < jsonArray.size(); ++i) {
        QJsonObject obj = jsonArray[i].toObject();
        // Get next available icon number
        int iconNumber = availableIcons[i % availableIcons.size()];
        obj["iconName"] = QString("%1-question-%2").arg(iconNumber).arg(iconName);
        jsonArray[i] = obj;
    }

    return jsonArray;
}
