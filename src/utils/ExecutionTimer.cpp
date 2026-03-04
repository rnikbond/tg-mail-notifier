//----------------------------------------------------------
#include "logger.h"
//----------------------------------------------------------
#include "ExecutionTimer.h"
//----------------------------------------------------------

ExecutionTimer::ExecutionTimer(std::string name)
    : n_name(name)
    , m_start(std::chrono::high_resolution_clock::now()) {
}
//----------------------------------------------------------------------------------------------------------------------

ExecutionTimer::~ExecutionTimer() {

    time_at      end     = std::chrono::high_resolution_clock::now();
    time_elapsed elapsed = end - m_start;

    log_info("{} at: {} msec", n_name, elapsed.count());
}
//----------------------------------------------------------------------------------------------------------------------
