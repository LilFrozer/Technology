//
// Created by Алексей Подоплелов on 03.03.2026.
//

#ifndef TECHNOLOGY_SERVER_H
#define TECHNOLOGY_SERVER_H

#include <Connection/ClientSession.h>

/*
    * Класс Server - основной сервер, принимающий подключения
*/
class Server
{
public:
    Server(boost::asio::io_context& io_context, short port)
        : m_acceptor_(io_context,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    {
        // m_acceptor_ инициализируется для прослушивания всех интерфейсов на указанном порту
        std::cout << "Сервер запущен на порту " << port << "\n";

        // Начинаем принимать подключения
        this->accept_connection();
    }
    void broadcast(const std::string& message);
    void disconnect_all();
    size_t getClientCount();
protected:
    void accept_connection();
    boost::asio::ip::tcp::acceptor m_acceptor_;                 // Акцептор для принятия подключений
    std::set<std::shared_ptr<ClientSession>> m_sessions_;       // Множество всех активных сессий
    std::mutex m_sessionMutex_;                                 // Мьютекс для защиты списка сессий
};


#endif //TECHNOLOGY_SERVER_H