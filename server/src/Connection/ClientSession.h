//
// Created by Алексей Подоплелов on 03.03.2026.
//

#ifndef TECHNOLOGY_CLIENTSESSION_H
#define TECHNOLOGY_CLIENTSESSION_H

#include <boost/asio.hpp>
#include <set>
#include <mutex>
#include <iostream>

// !!!Клиент!!!
class ClientSession : public std::enable_shared_from_this<ClientSession>
{
protected:
    boost::asio::ip::tcp::socket m_socket_;                 // !!!Сокет клиента!!!
    boost::asio::streambuf buffer_;                         // !!!Буфер для чтения данных!!!
    std::set<std::shared_ptr<ClientSession>> &sessions_;    // !!!Cсылка на список всех клиентов!!!
    std::mutex &sessionsMutex_;                             // !!!Мьютекс для блокировки при добавлении/удалении клиентов!!!
    void read_message();                                    // !!!Чтение сообщения!!!
public:
    ClientSession(boost::asio::ip::tcp::socket socket,
                std::set<std::shared_ptr<ClientSession>>& sessions,
                std::mutex& sessionsMutex) :
                m_socket_(std::move(socket)),
                sessions_(sessions),
                sessionsMutex_(sessionsMutex)
    {
        std::cout << "ClientSession\n";
    }
    ~ClientSession() {
        std::cout << "~ClientSession\n";
    }
    void start();
    void send_message(const std::string& message);
    void disconnect();
};


#endif //TECHNOLOGY_CLIENTSESSION_H