//
// Created by Алексей Подоплелов on 03.03.2026.
//

#include "Connection/Server.h"

/*
    * Отправляет сообщение всем подключенным клиентам
*/
void Server::broadcast(const std::string& message)
{
    // Захватываем мьютекс для безопасного доступа к списку сессий
    std::lock_guard<std::mutex> lock(this->m_sessionMutex_);

    // Проходим по всем сессиям и отправляем сообщение
    for (auto& session : this->m_sessions_) {
        session->send_message(message);
    }
}

/*
    * Отключает всех подключенных клиентов
*/
void Server::disconnect_all()
{
    // Захватываем мьютекс для безопасного доступа
    std::lock_guard<std::mutex> lock(this->m_sessionMutex_);

    // Проходим по всем сессиям и закрываем их
    for (auto& session : this->m_sessions_) {
        session->disconnect();
    }

    // Очищаем список сессий
    this->m_sessions_.clear();
}

/*
    * Возвращает количество подключенных клиентов
*/
size_t Server::getClientCount()
{
    std::lock_guard<std::mutex> lock(this->m_sessionMutex_);
    return this->m_sessions_.size();
}

/*
    * Асинхронно принимает новые подключения
    * Рекурсивно вызывает себя для постоянного ожидания подключений
*/
void Server::accept_connection()
{
    // Асинхронно ожидаем подключения
    this->m_acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
    {
        if (!ec)
        {
            // Успешно подключился новый клиент

            // Создаем новую сессию для клиента
            auto session = std::make_shared<ClientSession>(
                std::move(socket), this->m_sessions_, this->m_sessionMutex_);

            // Запускаем сессию
            session->start();
        }

        // Продолжаем принимать следующие подключения
        accept_connection();
    });
}