#include <thread>
#include <chrono>
#include <ctime>

class Operator
{
public:
                    Operator();
    std::time_t     processCall(unsigned processingTime);
    bool            isBusy();
private:
    bool m_isBusy;
};