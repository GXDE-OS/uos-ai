#ifndef MGMTDEFS_H
#define MGMTDEFS_H

enum KnowledgeBaseProcessStatus {
    Processing = 0,
    Succeed,
    ProcessingError,     //数据处理出错
    FileError           //文件错误
};

#endif
