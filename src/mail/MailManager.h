//----------------------------------------------------------
#ifndef MAILMANAGER_H
#define MAILMANAGER_H
//----------------------------------------------------------
#include <condition_variable>
#include <memory>
#include <thread>
//----------------------------------------------------------
#include "repository/IRepository.h"
//----------------------------------------------------------

/**
 * @brief Менеджер для сканирования электронной почты
 * 
 * Класс работает в отдельном потоке в режиме LongPolling.
 * 1 раз в минуту происходит запроc новых UID на почте, и, если появились новые сообщения,
 * они загружаются и отправляютс в telegram чат.
 */
class MailManager {

public:

    MailManager(std::shared_ptr<IRepository> repo);

public:

    void start();
    void stop();

private:

    std::thread             m_thread;
    std::mutex              m_mutex;
    std::condition_variable m_cond_wait;
    bool                    m_request_stop = {false};

    std::shared_ptr<IRepository> m_repo;

private:

    void run();
    void scan_mails();
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILMANAGER_H
