#ifndef __SYLAR_HTTP_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "sylar/thread.h"

/**
 * @brief HTTPServlet模块
 * @brief 提供 HTTP 请求路径到处理类的映射，用于规范化的 HTTP 消息处理流程
 * @brief HTTP Servlet 包括两部分：
 *             第一部分是 Servlet 对象，每个 Servlet 对象表示一种处理 HTTP 消息的方法；
 *             第二部分是 ServletDispatch，它包含一个请求路径到 Servlet 对象的映射，用于指定一个请求路径该用哪个 Servlet 来处理
 */

namespace sylar {
namespace http {


/**
 * @brief 纯虚基类，子类必须重写其虚方法 handleClient
 */
class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name)
        :m_name(name) {}
    virtual ~Servlet() {}
    /**
     * @brief 处理请求
     * @param request http请求
     * @param rsponse http响应
     * @param session http连接
     * @return  是否处理成功
     */
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) = 0;
                   
    const std::string& getName() const { return m_name;}
protected:
    /// 名称(一般用于打印日志区分不同的Servlet)
    std::string m_name;
};

/**
 * @brief 函数式Servlet --> 内含回调函数，其 handleClient 方法其实是直接调用回调函数
 */
class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;

    /// 函数回调类型的定义
    typedef std::function<int32_t (sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session)> callback;


    FunctionServlet(callback cb);
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) override;
private:
    callback m_cb;
};

/**
 * @brief Servlet分发器
 * @brief 内含精准匹配的 Servlet map 和 模糊匹配的 Servlet vector，并提供增、删、查方法
 */
class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) override;

    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    /**
     * @brief 添加模糊匹配Servlet
     * @param uri 模糊匹配 /sylar *
     * @param slt Servlet
     */
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    /**
     * @brief 添加模糊匹配Servlet
     * @param uri 模糊匹配 /sylar *
     * @param cb  FunctionServlet回调函数
     */
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default;}
    void setDefault(Servlet::ptr v) { m_default = v;}

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);

    /**
     * @brief 通过uri获取Servlet
     * @param uri uri
     * @return 优先精准匹配，其次模糊匹配，最后返回默认
     */
    Servlet::ptr getMatchedServlet(const std::string& uri);
private:
    /// 读写互斥量
    RWMutexType m_mutex;
    /// 精准匹配Servlet map
    //uri(/sylar/xxx) -> servlet
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    /// 模糊匹配Servlet 数组
    //uri(/sylar/*) -> servlet
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    //默认servlet，所有路径都没匹配到时使用
    Servlet::ptr m_default;
};

/**
 * @brief ServletDispatch 默认的 Servlet，404 那种
 */
class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                   , sylar::http::HttpResponse::ptr response
                   , sylar::http::HttpSession::ptr session) override;

};

}
}

#endif