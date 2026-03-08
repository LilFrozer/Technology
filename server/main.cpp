#include <Connection/Server.h>

int main()
{
    std::cout << "tech\n";
    try {
        // Создаем контекст ввода/вывода Asio
        boost::asio::io_context io_context;

        // Создаем сервер на порту 12345
        Server server(io_context, 12345);

        // Запускаем отдельный поток для обработки асинхронных операций Asio
        // Это позволяет основному потоку заниматься вводом команд
        std::thread io_thread([&io_context]() {
            io_context.run();  // Блокирующий вызов - обрабатывает все асинхронные операции
        });

        // Простой консольный интерфейс для управления сервером
        std::string command;
        std::cout << "Команды: broadcast <сообщение>, count, stop\n";

        while(true) {
            std::cout << "> ";                          // Приглашение к вводу
            std::getline(std::cin, command);      // Читаем команду

            if (command == "stop") {
                // Команда остановки сервера
                server.disconnect_all();            // Отключаем всех клиентов
                io_context.stop();                  // Останавливаем io_context
                break;                              // Выходим из цикла
            } else if (command == "count") {
                // Команда показа количества клиентов
                std::cout << "Подключено клиентов: " << server.getClientCount() << "\n";
            } else if (command.substr(0, 9) == "broadcast") {
                // Команда рассылки сообщения всем клиентам
                if (command.length() > 10) {
                    std::string message = command.substr(10);  // Извлекаем сообщение
                    server.broadcast("BROADCAST: " + message);  // Отправляем всем
                }
            }
        }

        // Ожидаем завершения потока Asio
        io_thread.join();
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}