#include <thread>
#include <chrono>
#include <ctime>

unsigned Operator::m_freeOperatorsCount = 0;

class Operator
{
public:
                    Operator();
                    ~Operator();
    std::time_t     processCall(unsigned processingTime);
    static unsigned m_freeOperatorsCount;

private:
    bool m_isBusy;
};