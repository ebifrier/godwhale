
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
    static void Initialize();

    /**
     * @brief シングルトンインスタンスを取得します。
     */
    static shared_ptr<Server> GetInstance()
    {
        return ms_instance;
    }

private:
    static shared_ptr<Server> ms_instance;

    explicit Server();

    void StartThread();
    void ServiceThreadMain();

    void BeginAccept();
    void HandleAccept(shared_ptr<tcp::socket> socket,
                      const system::error_code &error);

public:
    ~Server();

    std::list<shared_ptr<Client> > CloneClientList();
    
    void MakeRootMove(move_t move);
    void UnmakeRootMove();
    int Iterate(int *value, std::vector<move_t> &pvseq);

private:
    asio::io_service m_service;
    shared_ptr<thread> m_thread;
    Mutex m_guard;

    volatile bool m_isAlive;
    tcp::acceptor m_acceptor;

    std::list<weak_ptr<Client> > m_clientList;
    atomic<int> m_gid;
};

}
}

#endif
