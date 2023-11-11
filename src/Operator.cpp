#include "../inc/Operator.hpp"

std::time_t Operator::processCall(unsigned processingTime)
{
    std::chrono::milliseconds timespan(processingTime);
    std::this_thread::sleep_for(timespan);
    return std::time(0);
}