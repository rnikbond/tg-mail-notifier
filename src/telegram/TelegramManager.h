//----------------------------------------------------------
#ifndef TELEGRAMMANAGER_H
#define TELEGRAMMANAGER_H
//----------------------------------------------------------
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>
//----------------------------------------------------------
#include "httplib.h"
//----------------------------------------------------------
#include "TelegramController.h"
#include "repository/IRepository.h"
#include "telegram/TelegramAPI.h"
//----------------------------------------------------------

class TelegramManager {

public:

    TelegramManager(const std::string& token, const std::string& host_port, std::shared_ptr<IRepository> repo);
    ~TelegramManager();

    void start();
    void stop() noexcept;

    void send_msg(const TelegramRequest&& request);

private:

    std::string m_token;
    int64_t     m_last_chat_update_id = {-1};

    std::shared_ptr<IRepository> m_repo;

    bool                    m_request_stop = {false};
    std::mutex              m_mutex;
    std::condition_variable m_wait_cond;
    std::thread             m_thread;

    std::unique_ptr<httplib::Client>    m_http;
    std::unique_ptr<TelegramController> m_controller;

private:

    void run();
};
//----------------------------------------------------------------------------------------------------------------------

#endif // TELEGRAMMANAGER_H
