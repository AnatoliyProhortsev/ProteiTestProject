#include "../inc/Operator.hpp"

Operator::Operator()
    :m_isBusy(false){}

std::time_t Operator::processCall(unsigned processingTime)
{
    m_isBusy = true;
    std::chrono::milliseconds timespan(processingTime);
    std::this_thread::sleep_for(timespan);
    m_isBusy = false;
    return std::time(0);
}

bool Operator::isBusy(){return m_isBusy;};