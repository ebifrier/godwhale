#ifndef GODWHALE_LOGGER_H
#define GODWHALE_LOGGER_H

namespace godwhale {

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

#define LOG(severity)                                                   \
    BOOST_LOG_SEV(::godwhale::Logger::get(), severity)            \
        << boost::log::add_value("SrcFile", ::godwhale::getLogFileName(__FILE__))   \
        << boost::log::add_value("RecordLine", __LINE__)                \
        << boost::log::add_value("CurrentFunction", ::godwhale::getLogFuncName(__FUNCTION__))

#define LOG_DEBUG()        LOG(::godwhale::Debug)
#define LOG_NOTIFICATION() LOG(::godwhale::Notification)
#define LOG_WARNING()      LOG(::godwhale::Warning)
#define LOG_ERROR()        LOG(::godwhale::Error)
#define LOG_CRITICAL()     LOG(::godwhale::Critical)

/// ログを初期化します。
extern void InitializeLog();
extern std::string getLogFileName(const std::string &filepath);
extern std::string getLogFuncName(const std::string &funcname);

} // namespace godwhale

#endif
