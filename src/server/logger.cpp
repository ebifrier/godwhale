
#include "precomp.h"
#include "stdinc.h"

#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>   
#include <boost/log/support/date_time.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/attributes/current_process_name.hpp>

namespace godwhale {
namespace server {

using namespace boost::log;

/// グローバルロガーを設定する
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

static std::string GetLogName()
{
    posix_time::time_facet *f = new posix_time::time_facet("%Y%m%d_%H%M%S.log");

    std::ostringstream oss;
    oss.imbue(std::locale(oss.getloc(), f));

    oss << posix_time::second_clock::local_time();
    return oss.str();
}

void InitializeLog()
{
    typedef sinks::text_ostream_backend text_backend;
    auto backend = make_shared<text_backend>();
    backend->auto_flush(true);

    auto os = shared_ptr<std::ostream>(&std::cout, empty_deleter());
    backend->add_stream(os);

    auto logpath = "log/" + GetLogName();
    auto fs = shared_ptr<std::ostream>(new std::ofstream(logpath));
    backend->add_stream(fs);
    
    auto frontend = make_shared< sinks::synchronous_sink<text_backend> >(backend);
    //frontend->set_filter(Severity >= Notification);

    core::get()->remove_all_sinks();
    core::get()->add_sink(frontend);
}

} // namespace server
} // namespace godwhale
