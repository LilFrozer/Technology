#include <Bot.h>
#include <exception>
#include <string>

void TG::MovieBot::setupSSLContext()
{
    // Загружаем системные сертификаты
    ctx_.set_default_verify_paths();

    // На macOS дополнительно указываем путь к сертификатам Homebrew
#ifdef __APPLE__
    ctx_.add_verify_path("/opt/homebrew/etc/openssl@3/certs");
    ctx_.add_verify_path("/usr/local/etc/openssl@3/certs");
    ctx_.add_verify_path("/etc/ssl/certs");
#endif

    // Настраиваем верификацию сертификата
    ctx_.set_verify_mode(ssl::verify_peer);
    ctx_.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
        // Можно добавить дополнительную проверку здесь
        return preverified; // Возвращаем результат стандартной проверки
    });
}

void TG::MovieBot::run()
{
    std::cout << "STARTED!\n";
    while(true)
    {
        try
        {
            // -> getUpdates
            // Use this method to receive incoming updates using long polling
            // Returns an Array of Update objects.
            // ru: Формируем запрос на получение обновлений
            json payload =
            {
                {"offset", last_update_id_ + 1},
                {"timeout", 30},  // Long polling на 30 секунд
                {"allowed_updates", json::array({"message"})}
            };
            // https POST запрос к api
            auto response = this->sendRequest("getUpdates", payload);

            if (response.contains("ok") && response["ok"])
                this->processUpdate(response);
            else
                std::cerr << "API error: " << response.dump() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Run loop error: " << e.what() << std::endl;
        }
    }
}

json TG::MovieBot::sendRequest(const std::string &method, const json &payload)
{
    try {
        // Создаем поток, если его нет или он закрыт
        if (!stream_ || !stream_->lowest_layer().is_open())
        {
            stream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(ioc_, ctx_);

            // Устанавливаем SNI (Server Name Indication) - обязательно для Telegram
            if(!SSL_set_tlsext_host_name(stream_->native_handle(), TELEGRAM_API.c_str()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                throw beast::system_error{ec};
            }

            // Разрешаем доменное имя
            tcp::resolver resolver(ioc_);
            auto const results = resolver.resolve(TELEGRAM_API, TELEGRAM_PORT);

            // Подключаемся к серверу
            beast::get_lowest_layer(*stream_).connect(results);

            // Выполняем SSL handshake
            stream_->handshake(ssl::stream_base::client);
        }

        // Формируем HTTP-запрос
        std::string body = payload.dump();
        http::request<http::string_body> req{http::verb::post,
            "/bot" + token_ + "/" + method, 11};
        req.set(http::field::host, TELEGRAM_API);
        req.set(http::field::user_agent, "Boost Beast Telegram Bot");
        req.set(http::field::content_type, "application/json");
        req.set(http::field::content_length, std::to_string(body.size()));
        req.body() = body;

        // Отправляем запрос
        http::write(*stream_, req);

        // Получаем ответ
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(*stream_, buffer, res);

        // Проверяем статус ответа
        if (res.result() != http::status::ok)
        {
            throw std::runtime_error("HTTP error: " + std::to_string(res.result_int()));
        }

        // Парсим JSON
        return json::parse(res.body());

    }
    catch (const std::exception& e)
    {
        std::cerr << "Request error: " << e.what() << std::endl;
        // Пересоздаем поток при ошибке
        stream_.reset();
        throw;
    }
}

void TG::MovieBot::sendMultipartRequest(const std::string& method,
                                    const std::vector<char>& body,
                                    const std::string& boundary) {
    try {
        // Проверяем соединение
        if (!stream_ || !stream_->lowest_layer().is_open()) {
            stream_ = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(ioc_, ctx_);

            if(!SSL_set_tlsext_host_name(stream_->native_handle(), TELEGRAM_API.c_str())) {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
                throw beast::system_error{ec};
            }

            tcp::resolver resolver(ioc_);
            auto const results = resolver.resolve(TELEGRAM_API, TELEGRAM_PORT);
            beast::get_lowest_layer(*stream_).connect(results);
            stream_->handshake(ssl::stream_base::client);
        }

        // Формируем HTTP запрос
        http::request<http::vector_body<char>> req{http::verb::post,
            "/bot" + token_ + "/" + method, 11};
        req.set(http::field::host, TELEGRAM_API);
        req.set(http::field::user_agent, "Boost Beast YouTube Bot");
        req.set(http::field::content_type, "multipart/form-data; boundary=" + boundary);
        req.set(http::field::content_length, std::to_string(body.size()));
        req.body() = body;

        // Отправляем запрос
        http::write(*stream_, req);

        // Получаем ответ
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(*stream_, buffer, res);

        if (res.result() != http::status::ok) {
            throw std::runtime_error("HTTP error: " + std::to_string(res.result_int()) +
                                   ", body: " + res.body());
        }

        // Парсим ответ
        json response = json::parse(res.body());
        if (!response.value("ok", false)) {
            throw std::runtime_error("Telegram API error: " + response.dump());
        }

        std::cout << "✅ Video sent successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Multipart request error: " << e.what() << std::endl;
        stream_.reset();
        throw;
    }
}

void TG::MovieBot::sendMessage(int64_t chat_id, const std::string &text)
{
    json payload = {
        {"chat_id", chat_id},
        {"text", text},
        {"parse_mode", "HTML"}
    };

    try
    {
        auto response = this->sendRequest("sendMessage", payload);
        if (!response.contains("ok") || !response["ok"])
        {
            std::cerr << "Failed to send message: " << response.dump() << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Send message error: " << e.what() << std::endl;
    }
}

#include <random>
#include <fstream>

void TG::MovieBot::processMessage(const json& message)
{
    auto executeCmd = [&](const std::string &cmd) -> std::string
    {
        std::array<char, 128> buffer;
        std::string result;

        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) {
            return "";
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        // Удаляем trailing newline
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }

        return result;
    };

    int64_t chat_id = message["chat"]["id"];
    std::string url = message.value("text", "");

    std::cout << "Received from " << chat_id << ": " << url << std::endl;

    if (this->m_activeDowloaders_[chat_id])
    {
        this->sendMessage(chat_id, "Пожалуйста, дождитесь завершения текущей загрузки.");
        return;
    }

    this->m_activeDowloaders_[chat_id] = true;

    try
    {
        this->sendMessage(chat_id, "Загружаю видео...");

        std::string path = "/Users/alekseypodoplelov/Documents/tg-bot/tmp/1";
        std::string cmd = "yt-dlp -f \"b[filesize<50M] / w\" -S \"+size\" --merge-output-format mp4 -o \"" + path + ".mp4\" \"" + url + "\"";

        std::string output = executeCmd(cmd);

        if (output.find("ERROR") != std::string::npos)
        {
            throw std::runtime_error("Download failed: " + output);
        }

        this->sendMessage(chat_id, "Отправляю видео в Telegram...");

        auto generateBoundary = [&]() -> std::string
        {
            const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, chars.size() - 1);

            std::string boundary = "----BoostBeastTelegramBot";
            for (int i = 0; i < 16; ++i) {
                boundary += chars[dis(gen)];
            }
            return boundary;
        };

        auto readFileBytes = [&](const std::string &filepath) -> std::vector<char>
        {
            std::ifstream file(filepath, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file: " + filepath);
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<char> buffer(size);
            if (!file.read(buffer.data(), size)) {
                throw std::runtime_error("Cannot read file: " + filepath);
            }

            return buffer;
        };

        std::vector<char> video_data = readFileBytes(path + ".mp4");

        // Генерируем boundary
        std::string boundary = generateBoundary();

        // Создаем multipart тело запроса
        std::string multipart_body;

        // Добавляем поле chat_id
        multipart_body += "--" + boundary + "\r\n";
        multipart_body += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
        multipart_body += std::to_string(chat_id) + "\r\n";

        // Добавляем поле caption
        multipart_body += "--" + boundary + "\r\n";
        multipart_body += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
        multipart_body += "Ваше видео\r\n";

        // Добавляем поле parse_mode
        multipart_body += "--" + boundary + "\r\n";
        multipart_body += "Content-Disposition: form-data; name=\"parse_mode\"\r\n\r\n";
        multipart_body += "HTML\r\n";

        // Добавляем поле supports_streaming
        multipart_body += "--" + boundary + "\r\n";
        multipart_body += "Content-Disposition: form-data; name=\"supports_streaming\"\r\n\r\n";
        multipart_body += "true\r\n";

        // Добавляем видео файл
        multipart_body += "--" + boundary + "\r\n";
        multipart_body += "Content-Disposition: form-data; name=\"video\"; filename=\"1\"\r\n";
        multipart_body += "Content-Type: video/mp4\r\n";
        multipart_body += "Content-Length: " + std::to_string(video_data.size()) + "\r\n\r\n";

        // Создаем финальный буфер
        std::vector<char> request_body;
        request_body.insert(request_body.end(), multipart_body.begin(), multipart_body.end());
        request_body.insert(request_body.end(), video_data.begin(), video_data.end());

        // Добавляем завершающий boundary
        std::string end_boundary = "\r\n--" + boundary + "--\r\n";
        request_body.insert(request_body.end(), end_boundary.begin(), end_boundary.end());

        this->sendMultipartRequest("sendVideo", request_body, boundary);
        this->m_activeDowloaders_[chat_id] = false;
        std::filesystem::remove(path + ".mp4");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        this->m_activeDowloaders_[chat_id] = false;
    }
}

void TG::MovieBot::processUpdate(const json &updates)
{
    if (!updates.contains("result") || !updates["result"].is_array())
    {
        return;
    }

    for (const auto& update : updates["result"])
    {
        int64_t update_id = update["update_id"];

        // Пропускаем старые обновления
        if (update_id <= last_update_id_)
        {
            continue;
        }

        // std::cout << "JSON content: " << updates << std::endl;

        // Обрабатываем сообщение
        if (update.contains("message"))
        {
            this->processMessage(update["message"]);
        }

        last_update_id_ = update_id;
    }
}
