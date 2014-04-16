
#ifndef GODWHALE_SERVER_CLIENT_H
#define GODWHALE_SERVER_CLIENT_H

namespace godwhale {
namespace server {

class Server;

/**
 * @brief リスナーＰＣによるクライアントを管理します。
 */
class Client : public enable_shared_from_this<Client>
{
public:
    explicit Client(Server *server, shared_ptr<tcp::socket> socket);
    ~Client();

    void BeginAsyncReceive();

    void SendCommand(const std::string &command);

private:
    void Disconnected();
    
    void HandleAsyncReceive(const system::error_code &error);

    void PutSendCommand(const std::string &command);
    std::string GetSendCommand();

    void BeginAsyncSend();
    void HandleAsyncSend(const system::error_code &error);

    int ParseCommand(const std::string &command);

private:
    Server *m_server;
    shared_ptr<tcp::socket> m_socket;
    Mutex m_guard;
    asio::streambuf m_streambuf;

    std::list<std::string> m_sendList;
    std::string m_sendingbuf;

    std::string m_id;
    int m_nthreads;
    bool m_sendpv;

    move_t m_move;
    bool m_stable;
    bool m_final;
    long m_nodes;
    int m_value;
    int m_pid;
};

} // namespace server
} // namespace godwhale

#endif
