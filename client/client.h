#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <boost/asio.hpp>
#include <thread>
#include <QDateTime>
#include <iostream>

namespace Constants
{
const unsigned SERVER_PORT = 12345;
}

QT_BEGIN_NAMESPACE
namespace Ui {
class Client;
}
QT_END_NAMESPACE

class Client : public QMainWindow
{
    Q_OBJECT
private:
    Client(QWidget *parent = nullptr);
public:
    static Client &get_instance()
    {
        static Client client;
        return client;
    }
    ~Client();
public slots:
    void logMessage(QString text);
    void onDisconnect();
private:
    Ui::Client *ui;
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    boost::asio::ip::tcp::socket socket_;   // Сокет для связи с сервером
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::streambuf buffer_;         // Буфер для чтения данных
    std::unique_ptr<std::thread> io_thread_;
    bool connected_ = false;
private:
    void read_message();
private slots:
    void onConnect();
    void onSendMessage();
};

#endif // CLIENT_H
