
#ifndef GODWHALE_SERVER_SERVER_H
#define GODWHALE_SERVER_SERVER_H

namespace godwhale {
namespace server {

class Client;

/**
 *
 */
class Server : public enable_shared_from_this<Server>
{
public:
    /**
     * @brief シングルトンインスタンスを取得します。
     */
    static shared_ptr<Server> GetInstance()
    {
        mutex::scoped_lock lock(ms_guard);

        if (ms_instance == NULL) {
            ms_instance = shared_ptr<Server>(new Server());
        }

        return ms_instance;
    }

private:
    static shared_ptr<Server> ms_instance;
    static mutex ms_guard;

    explicit Server();

    void ServiceThreadMain();

    void BeginAccept();
    void HandleAccept(shared_ptr<tcp::socket> socket,
                      const system::error_code &error);

public:
    ~Server();

    void ClientDisconnected(shared_ptr<Client> client);
    
    int Iterate(int *value, std::vector<move_t> &pvseq);

private:
    asio::io_service m_service;
    shared_ptr<thread> m_thread;
    Mutex m_guard;

    volatile bool m_isAlive;
    tcp::acceptor m_acceptor;

    std::list<shared_ptr<Client> > m_clientList;
};

}
}

#endif
