
#include "precomp.h"
#include "stdinc.h"
#include "logger.h"

#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>   
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/current_process_name.hpp>

namespace godwhale {

using namespace boost::log;
using namespace boost::posix_time;

/**
 * @brief グローバルロガーを設定する。
 */
BOOST_LOG_GLOBAL_LOGGER_INIT(Logger, sources::severity_logger_mt<SeverityLevel>)
{
    //ログレコードの属性を設定する
    auto r = sources::severity_logger_mt<SeverityLevel>();
    
    //[ログに含まれる属性を設定する]
    //以下の共通属亭定義関数で定義する事も可能である。
    //boost::log::add_common_attributes();
    //
    //ここでは、独自に必要な属性の定義を行う
    r.add_attribute("TimeStamp", attributes::local_clock());
    r.add_attribute("ProcessID", attributes::current_process_id());
    r.add_attribute("Process", attributes::current_process_name());
    r.add_attribute("ThreadID", attributes::current_thread_id());

    return std::move(r);
}

/**
 * @brief ファイルパスから名前のみを取り出します。
 */
std::string getLogFileName(const std::string &filepath)
{
    char separator =
#if defined(_WIN32)
        '\\';
#else
        '/';
#endif

    auto i = filepath.find_last_of(separator);
    return (i != std::string::npos ? filepath.substr(i+1) : filepath);
}

/**
 * @brief ログ出力用の関数名を取り出します。
 */
std::string getLogFuncName(const std::string &funcname)
{
    auto i = funcname.find_last_of(':');
    return (i != std::string::npos ? funcname.substr(i+1) : funcname);
}

/**
 * @brief ログの出力レベル名を取得します。
 */
std::string getSeverityName(SeverityLevel severity)
{
    switch (severity) {
    case Debug:
        return "DEBUG";
    case Notification:
        return "NOTIFICATION";
    case Warning:
        return "WARNING";
    case Error:
        return "ERROR";
    case Critical:
        return "CRITICAL";
    }

    return "UNKNOWN";
}

/**
 * @brief ログファイル名にはタイムスタンプを使います。
 */
static std::string getLogName()
{
    time_facet *f = new time_facet("%Y%m%d_%H%M%S.log");

    std::ostringstream oss;
    oss.imbue(std::locale(oss.getloc(), f));

    oss << second_clock::local_time();
    return oss.str();
}

/**
 * @brief 何もしないdeleterです。
 */
struct empty_deleter
{
    typedef void result_type;
    void operator() (const volatile void*) const {}
};

/**
 * @brief 対局開始毎にログファイルの初期化を行います。
 */
void initializeLog()
{
    typedef sinks::text_ostream_backend text_backend;
    auto backend = make_shared<text_backend>();
    backend->auto_flush(true);

    // コンソール出力
#if !defined(USI)
    auto os = shared_ptr<std::ostream>(&std::cout, empty_deleter());
    backend->add_stream(os);
#endif

    // ファイル出力
    auto logpath =
#if defined(GODWHALE_SERVER)
        "server.log";
#else
        "client.log";
#endif
    auto fs = shared_ptr<std::ostream>(new std::ofstream(logpath, std::ios::app));
    backend->add_stream(fs);

    // 通常ログ
    auto normal = make_shared< sinks::synchronous_sink<text_backend> >(backend);
    //error->set_filter(Severity >= Warning);
    normal->set_formatter
        ( expressions::format("[%1%]: %2%")
        % expressions::format_date_time<ptime>
            ("TimeStamp", "%Y-%m-%d %H:%M:%S")
        % expressions::message);
    
    // エラー用出力
    auto error = make_shared< sinks::synchronous_sink<text_backend> >(backend);
    error->set_filter(Severity >= Warning);
    error->set_formatter
        ( expressions::format("[%1%]:%2%(%3%):%4%:%5%: %6%")
        % expressions::format_date_time<ptime>
            ("TimeStamp", "%Y-%m-%d %H:%M:%S")
        % expressions::attr<std::string>("SrcFile")
        % expressions::attr<std::string>("RecordLine")
        % expressions::attr<std::string>("CurrentFunction")
        % expressions::attr<std::string>("SeverityName")
        % expressions::message);

    core::get()->remove_all_sinks();
    core::get()->add_sink(normal);
    core::get()->add_sink(error);
}

} // namespace godwhale
