export enum ErrorType {
    NoError = 0,

    HttpError = 1,

    // session eror (1000 - 1199)
    InvalidSession = 1000,

    // assistant error (1200 - 1399)
    InvalidAssistant = 1200,

    // model error (1400 - 1499)
    InvalidModel = 1400,
    ModelExpired = 1401,
    ModelUsageLimitReached = 1402,
    ModelChatUsageLimitReached = 1403,
    ModelChatUsageClaimAgain = 1404,

    // file errors (1500 - 1599)
    FileInvalidSuffix = 1500,
    FileSizeExceeded = 1501,
    FileImageSizeExceeded = 1502,
    FileParseNoText = 1503,
    FileParseFailed = 1504,

    // mcp error (1600 - 1699)
    AgentServerUnavailable = 1600,
    AgentServerInvaildContent,
    MCPSeverUnavailable,
    MCPToolError,

    // audio errors (1700 - 1799)
    AudioInputDeviceInvalid = 1700,
    AudioOutputDeviceInvalid = 1701,
    AudioNetworkError = 1702,

    // knowledge base errors (1800 - 1899)
    KnowledgeBasePluginNotInstalled = 1801,
    KnowledgeBaseEmpty = 1802,
    KnowledgeBaseEmptyQuery = 1803,
}

export enum HttpErrorType {
    ConnectionRefusedError = 1,
    RemoteHostClosedError,
    HostNotFoundError,
    TimeoutError,
    OperationCanceledError,
    SslHandshakeFailedError,
    TemporaryNetworkFailureError,
    NetworkSessionFailedError,
    BackgroundRequestNotAllowedError,
    TooManyRedirectsError,
    InsecureRedirectError,
    UnknownNetworkError = 99,
}
