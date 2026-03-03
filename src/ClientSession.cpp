//
// Created by Алексей Подоплелов on 03.03.2026.
//

#include "Connection/ClientSession.h"

void ClientSession::read_message() {
    // Получаем shared_ptr на себя для безопасности
    auto self = shared_from_this();

    // Асинхронно читаем данные до символа новой строки
    boost::asio::async_read_until(socket_, buffer_, '\n',
        [this, self](boost::system::error_code ec, std::size_t length)
        {
            if (!ec) {
                // Успешно прочитали данные

                // Извлекаем данные из буфера
                std::string data{
                    boost::asio::buffers_begin(buffer_.data()),
                    boost::asio::buffers_begin(buffer_.data()) + length
                };

                // Удаляем прочитанные данные из буфера
                buffer_.consume(length);

                // Убираем символ новой строки из конца сообщения
                if (!data.empty() && data.back() == '\n') {
                    data.pop_back();
                }

                // Выводим полученное сообщение
                std::cout << "Получено сообщение от клиента: " << data << "\n";

                // Отправляем подтверждение клиенту
                std::string response = "Сервер получил: " + data;
                send_message(response);

                // Продолжаем чтение следующих сообщений
                read_message();
            } else {
                // Произошла ошибка или клиент отключился

                // Удаляем сессию из списка активных
                {
                    std::lock_guard<std::mutex> lock(sessions_mutex_);
                    sessions_.erase(self);  // Удаляем себя из списка
                }

                // Выводим информацию об отключении
                std::cout << "Клиент отключился. Всего клиентов: "
                          << sessions_.size() << "\n";
            }
        });
}