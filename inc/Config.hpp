#include <fstream>

#include "../lib/json/json.hpp"

using json = nlohmann::json;

class Config
{
public:
                Config();
    bool        readConfigFile(std::string fileName);
    bool        getCallDuplicationMode();
    unsigned    getOperatorsCount();
    unsigned    getQueueSize();
    unsigned    getRmin();
    unsigned    getRmax();
    unsigned    getProcessingTime();
    void        performDefaultCfg();
private:
    unsigned m_operatorsCount;
    unsigned m_queueSize;
    unsigned m_Rmin;
    unsigned m_Rmax;
    unsigned m_processingTime;
    bool     m_callDuplicationMode; //false for error, true for call-closing
};