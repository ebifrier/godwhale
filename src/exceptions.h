
#ifndef GODWHALE_EXCEPTION_H
#define GODWHALE_EXCEPTION_H

namespace godwhale {

/**
 * @brief SFEN形式の連続した指し手文字列を、実際の指し手に変換します。
 */
class SfenException : public std::exception
{
public:
    explicit SfenException(std::string const & message)
        : m_message(message)
    {
    }

    explicit SfenException(char const * fmt, ...)
    {
        char buffer[1024];
        va_list vlist;

        va_start(vlist, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, vlist);
        va_end(vlist);

        m_message = buffer;
    }

    virtual ~SfenException() throw()
    {
    }

    virtual const char* what() const throw()
    {
        return m_message.c_str();
    }

private:
    std::string m_message;
};

} // namespace godwhale

#endif
