#include "client.h"
#include "ui_client.h"

Client::Client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Client)
    , io_context_()
    , work_guard_(boost::asio::make_work_guard(io_context_))
    , socket_(io_context_)
    , resolver_(io_context_)
    , connected_(false)
{
    ui->setupUi(this);
    QObject::connect(ui->wdgt_bttn_connect, &QPushButton::clicked, this, &Client::onConnect);
    QObject::connect(ui->wdgt_bttn_disconnect, &QPushButton::clicked, this, &Client::onDisconnect);
    QObject::connect(ui->wdgt_bttn_send_message, &QPushButton::clicked, this, &Client::onSendMessage);

    io_thread_ = std::make_unique<std::thread>([this]()
    {
        try {
            io_context_.run();
        } catch (const std::exception& e) {
            qDebug() << "Ошибка в потоке io_context:" << e.what();
        }
    });
}

Client::~Client()
{
    if (connected_) {
        this->onDisconnect();
    }

    // Останавливаем io_context и ждем завершения потока
    work_guard_.reset();
    io_context_.stop();
    if (io_thread_ && io_thread_->joinable()) {
        io_thread_->join();
    }

    delete ui;
}

void Client::logMessage(QString text)
{
    QString logText = QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "|" + text;
    ui->wdgt_console->appendPlainText(logText);
}

void Client::onConnect()
{
    QString host = ui->wdgt_addr->text();
    if (host.isEmpty()) {
        logMessage("Ошибка: введите адрес сервера");
        return;
    }

    logMessage("Подключение к " + host + ":" + QString::number(Constants::SERVER_PORT));

    try {
        // Асинхронно разрешаем адрес и подключаемся
        auto endpoints = resolver_.resolve(ui->wdgt_addr->text().toStdString(), std::to_string(Constants::SERVER_PORT));

        // Асинхронное подключение к серверу
        boost::asio::async_connect(socket_, endpoints,
            [this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint)
        {
            if (!ec) {
                connected_ = true;

                QString c  = "Подключено к серверу";
                QMetaObject::invokeMethod
                (
                    &Client::get_instance(),
                    "logMessage",
                    Qt::QueuedConnection,
                    Q_ARG(QString, c)
                );

                // Начинаем чтение сообщений от сервера
                this->read_message();
            } else {
                connected_ = false;

                QString c  = "Ошибка подключения: " + QString::fromStdString(ec.message());
                QMetaObject::invokeMethod
                (
                    &Client::get_instance(),
                    "logMessage",
                    Qt::QueuedConnection,
                    Q_ARG(QString, c)
                );
            }
        });
    } catch (const std::exception &e) {
        QString c = "Ошибка: " + QString::fromStdString(e.what());
        QMetaObject::invokeMethod
        (
            &Client::get_instance(),
            "logMessage",
            Qt::QueuedConnection,
            Q_ARG(QString, c)
        );
    }
}

/*
    * Асинхронно читает сообщения от сервера
    * Рекурсивно вызывает себя для постоянного чтения
*/
void Client::read_message()
{
    // Асинхронно читаем данные до символа новой строки
    boost::asio::async_read_until(socket_, buffer_, '\n',
        [this](boost::system::error_code ec, std::size_t length)
    {
        if (ec == boost::asio::error::operation_aborted) {
            // Операция отменена при disconnect - это нормально
            return;
        }
        else if (!ec) {
            // Успешно получили сообщение

            // Извлекаем данные из буфера
            std::string data
            {
                boost::asio::buffers_begin(buffer_.data()),
                boost::asio::buffers_begin(buffer_.data()) + length
            };

            // Удаляем прочитанные данные из буфера
            buffer_.consume(length);

            // Убираем символ новой строки
            if (!data.empty() && data.back() == '\n') {
                data.pop_back();
            }

            // Выводим сообщение от сервера
            QString c = "Сервер: " + QString::fromStdString(data);
            QMetaObject::invokeMethod
            (
                &Client::get_instance(),
                "logMessage",
                Qt::QueuedConnection,
                Q_ARG(QString, c)
            );

            // Продолжаем чтение следующих сообщений
            this->read_message();
        } else if (ec != boost::asio::error::eof) {
            // Другая ошибка

            QString c = "Ошибка чтения: " + QString::fromStdString(ec.message());
            QMetaObject::invokeMethod
            (
                &Client::get_instance(),
                "logMessage",
                Qt::QueuedConnection,
                Q_ARG(QString, c)
            );

            QMetaObject::invokeMethod
            (
                &Client::get_instance(),
                "onDisconnect",
                Qt::QueuedConnection
            );
        }
    });
}

void Client::onDisconnect()
{
    // Выполняем закрытие сокета в потоке io_context
    boost::asio::post(io_context_, [this]()
    {
        if (socket_.is_open()) {
            boost::system::error_code ec;

            // Отменяем все операции
            socket_.cancel(ec);

            // Закрываем сокет
            socket_.close(ec);

            // Очищаем буфер
            buffer_.consume(buffer_.size());
        }

        connected_ = false;
    });

    logMessage("Отключение от сервера...");
}
void Client::onSendMessage()
{
    if (!socket_.is_open() || !connected_) {
        logMessage("Ошибка: нет подключения к серверу");
        return;
    }

    // Асинхронная отправка данных
    boost::asio::async_write(socket_,
        boost::asio::buffer(ui->wdgt_message->text().toStdString() + "\n"),  // Добавляем символ новой строки
        [this](boost::system::error_code ec, std::size_t /*length*/)
    {
        if (!ec) {
            // ui->wdgt_message->clear();
        } else {

            QString c = "Ошибка отправки: " + QString::fromStdString(ec.message());
            QMetaObject::invokeMethod
            (
                &Client::get_instance(),
                "logMessage",
                Qt::QueuedConnection,
                Q_ARG(QString, c)
            );

            if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                QMetaObject::invokeMethod
                (
                    &Client::get_instance(),
                    "onDisconnect",
                    Qt::QueuedConnection
                );
            }
        }
    });
}
