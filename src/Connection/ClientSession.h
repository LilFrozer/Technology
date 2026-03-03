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
class ClientSession : public std::enable_shared_from_this<ClientSession> {
private:
    boost::asio::ip::tcp::socket m_socket_;     // !!!Сокет клиента!!!
    boost::asio::streambuf buffer_;     // !!!Буфер для чтения данных!!!

    // !!!Cсылка на список всех клиентов!!!
    std::set<std::shared_ptr<ClientSession>> &sessions_;
    // !!!Мьютекс для блокировки при добавлении/удалении клиентов!!!
    std::mutex &sessionsMutex_;

    void read_message();
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
    void start() {
        // Добавляем текущую сессию в список активных
        {
            // Захватываем мьютекс для безопасного доступа к общему списку сессий
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            sessions_.insert(shared_from_this());  // Добавляем shared_ptr на себя
        }
        std::cout << "Новый клиент подключился. Всего клиентов: "  << getClientCount() << "\n";

        // Начинаем асинхронное чтение сообщений от клиента
        read_message();
    }
    void send_message(const std::string& message) {
        auto self = shared_from_this();
        boost::asio::async_write(m_socket_,
            boost::asio::buffer(message + "\n"),  // Добавляем символ новой строки как разделитель
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (ec)
                {
                    // Если ошибка - выводим сообщение
                    std::cerr << "Ошибка отправки: " << ec.message() << "\n";
                }
            });
    }
    void disconnect()
    {
        boost::system::error_code ec;       // Для игнорирования ошибок закрытия
        this->m_socket_.close(ec);       // Закрываем сокет
    }
    size_t getClientCount()
    {
        std::lock_guard<std::mutex> lock(this->sessionsMutex_);
        return sessions_.size();
    }
};


#endif //TECHNOLOGY_CLIENTSESSION_H