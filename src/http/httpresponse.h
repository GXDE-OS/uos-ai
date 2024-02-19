#ifndef _HTTP_RESPONSE_H
#define _HTTP_RESPONSE_H

#include <iostream>
#include <vector>

class HttpResponse final
{
public:
    HttpResponse(long code, std::vector<char> &&data)
        : data_(std::move(data)), code_(code)
    {
    }

#ifdef EWS_HAS_DEFAULT_AND_DELETE
    ~HttpResponse() = default;

    HttpResponse(const http_response &) = delete;
    HttpResponse &operator=(const http_response &) = delete;
#else
    HttpResponse() {}

private:
    HttpResponse(const HttpResponse &);           // Never defined
    HttpResponse &operator=(const HttpResponse &); // Never defined

public:
#endif

    HttpResponse(HttpResponse &&other)
        : data_(std::move(other.data_))
        , code_(std::move(other.code_))
        , error(other.error)
    {
        other.code_ = 0U;
    }

    HttpResponse &operator=(HttpResponse &&rhs)
    {
        if (&rhs != this) {
            data_ = std::move(rhs.data_);
            code_ = std::move(rhs.code_);
        }
        return *this;
    }

    // Returns a reference to the raw byte content in this HTTP
    // response.
    std::vector<char> &content() { return data_; }

    // Returns a reference to the raw byte content in this HTTP
    // response.
    const std::vector<char> &content() const  { return data_; }

    // Returns the response code of the HTTP request.
    long code() const { return code_; }

    // Returns whether the response is a SOAP fault.
    //
    // This means the server responded with status code 500 and
    // indicates that the entire request failed (not just a normal EWS
    // error). This can happen e.g. when the request we sent was not
    // schema compliant.
    bool isSoapFault() const { return code() == 500U; }

    // Returns whether the HTTP response code is 200 (OK).
    bool ok() const { return code() == 200U; }

    void setErrorCode(int code) { error = code; }
    int errorCode() const { return error; }

private:
    std::vector<char> data_;
    long code_ = 0U;

    int error = 0;
};

#endif
