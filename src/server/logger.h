#ifndef GODWHALE_SERVER_LOGGER_H
#define GODWHALE_SERVER_LOGGER_H

namespace godwhale {
namespace server {

/**
 * @brief ログレベル
 */
enum SeverityLevel
{
    Debug,
    Notification,
    Warning,
    Error,
    Critical,
};

BOOST_LOG_ATTRIBUTE_KEYWORD(Severity, "Severity", SeverityLevel);

BOOST_LOG_GLOBAL_LOGGER(Logger, boost::log::sources::severity_logger_mt<SeverityLevel>);

#define LOG(severity) BOOST_LOG_SEV(godwhale::server::Logger::get(), severity) \
    << boost::log::add_value("RecordLine", __LINE__)			\
    << boost::log::add_value("SrcFile", __FILE__)				\
    << boost::log::add_value("CurrentFunction", BOOST_CURRENT_FUNCTION)

/// ログを初期化します。
extern void InitializeLog();

} // namespace server
} // namespace godwhale

#endif
