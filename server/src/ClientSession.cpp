//
// Created by Алексей Подоплелов on 03.03.2026.
//

#include "Connection/ClientSession.h"

/*
    * Асинхронно читает сообщения от клиента
    * Рекурсивно вызывает себя для постоянного чтения
*/
void ClientSession::read_message()
{
    // Получаем shared_ptr на себя для безопасности
    auto self = shared_from_this();

    // Асинхронно читаем данные до символа новой строки
    boost::asio::async_read_until(this->m_socket_, this->buffer_, '\n',
    [this, self](boost::system::error_code ec, std::size_t length)
    {
        if (!ec)
        {
            // Успешно прочитали данные

            // Извлекаем данные из буфера
            std::string data
            {
                boost::asio::buffers_begin(buffer_.data()),
                boost::asio::buffers_begin(buffer_.data()) + length
            };

            // Удаляем прочитанные данные из буфера
            buffer_.consume(length);

            // Убираем символ новой строки из конца сообщения
            if (!data.empty() && data.back() == '\n')
                data.pop_back();

            // Выводим полученное сообщение
            std::cout << "Получено сообщение от клиента: " << data << "\n";

            // Отправляем подтверждение клиенту
            std::string response = "Сервер получил: " + data;
            send_message(response);

            // Продолжаем чтение следующих сообщений
            this->read_message();
        }
        else
        {
            // Произошла ошибка или клиент отключился

            // Удаляем сессию из списка активных
            {
                std::lock_guard<std::mutex> lock(this->sessionsMutex_);
                sessions_.erase(self);  // Удаляем себя из списка
            }

            // Выводим информацию об отключении
            std::cout << "Клиент отключился. Всего клиентов: " << sessions_.size() << "\n";
        }
    });
}

/*
    * Запускает обработку сессии - добавляет в список и начинает чтение
*/
void ClientSession::start()
{
    // Добавляем текущую сессию в список активных
    {
        // Захватываем мьютекс для безопасного доступа к общему списку сессий
        std::lock_guard<std::mutex> lock(sessionsMutex_);
        sessions_.insert(shared_from_this());  // Добавляем shared_ptr на себя
    }
    std::cout << "Новый клиент подключился.\n";

    // Начинаем асинхронное чтение сообщений от клиента
    this->read_message();
}

void ClientSession::disconnect()
{
    boost::system::error_code ec;       // Для игнорирования ошибок закрытия
    this->m_socket_.close(ec);       // Закрываем сокет
}

/*
     * Отправляет сообщение клиенту
     * const std::string& message - текст сообщения для отправки
*/
void ClientSession::send_message(const std::string& message)
{
    auto self = shared_from_this();
    boost::asio::async_write(m_socket_, boost::asio::buffer(message + "\n"),  // Добавляем символ новой строки как разделитель
    [this, self](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (ec)
        {
            // Если ошибка - выводим сообщение
            std::cerr << "Ошибка отправки: " << ec.message() << "\n";
        }
    });
}