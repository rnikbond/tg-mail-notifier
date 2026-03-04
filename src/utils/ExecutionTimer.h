//----------------------------------------------------------
#ifndef EXECUTIONTIMER_H
#define EXECUTIONTIMER_H
//----------------------------------------------------------
#include <chrono>
#include <string>
//----------------------------------------------------------

class ExecutionTimer {

    using time_at      = std::chrono::time_point<std::chrono::high_resolution_clock>;
    using time_elapsed = std::chrono::duration<double, std::milli>;

public:

    ExecutionTimer(std::string name);
    ~ExecutionTimer();

private:

    std::string n_name;
    time_at     m_start;
};
//----------------------------------------------------------------------------------------------------------------------

#endif // EXECUTIONTIMER_H
