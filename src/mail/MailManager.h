//----------------------------------------------------------
#ifndef MAILMANAGER_H
#define MAILMANAGER_H
//----------------------------------------------------------
#include <condition_variable>
#include <expected>
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
};
//----------------------------------------------------------------------------------------------------------------------

#endif // MAILMANAGER_H
