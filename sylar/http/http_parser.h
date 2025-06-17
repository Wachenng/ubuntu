#ifndef __SYLAR_HTTP_PARSER_H__
#define __SYLAR_HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar {
namespace http {

/**
 * @brief http请求解析类
 */
class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();

    /**
     * @brief 解析协议函数
     * @param data 协议文本缓存
     * @param len 协议文本缓存长度
     * @return 返回实际解析的长度，并且将已解析的数据移除
     */

    size_t execute(char* data, size_t len);
    /**
     * @brief 是否解析完成
     */
    int isFinished();

    /**
     * @brief 判断是否存在解析错误
     */
    int hasError(); 

    HttpRequest::ptr getData() const { return m_data;}
    void setError(int v) { m_error = v;}

    uint64_t getContentLength();
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    /// 错误码
    //1000: invalid method
    //1001: invalid version
    //1002: invalid field
    int m_error;
};


/**
 * @brief http响应解析类
 */
class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    /**
     * @brief 解析http响应协议
     *
     * @param data 协议数据缓存
     * @param len 协议数据缓存大小
     * @return 返回实际解析的长度，并且移除已解析的数据
     */
    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError(); 

    HttpResponse::ptr getData() const { return m_data;}
    void setError(int v) { m_error = v;}

    uint64_t getContentLength();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    //1001: invalid version
    //1002: invalid field
    int m_error;
};

}
}

#endif