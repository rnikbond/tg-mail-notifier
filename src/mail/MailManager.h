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

class MailManager {

public:

    MailManager(std::shared_ptr<IRepository> repo);

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

    void scan_emails();

    std::optional<std::vector<int64_t>> load_uids(const Email& email);
    std::optional<std::string> load_email_msg(const Email& email, int64_t uid);
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILMANAGER_H
