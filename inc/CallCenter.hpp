#include <queue>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <thread>
#include <iterator>
#include <vector>

#include "../lib/httpparser/httprequestparser.h"
#include "../lib/httpparser/request.h"
#include "../lib/json/json.hpp"

#include "Call.hpp"
#include "CallDetailedRecord.hpp"
#include "Config.hpp"
#include "Operator.hpp"

struct CDR
    {
        std::time_t m_callReceiveDT;
        std::time_t m_callAnswerDT;
        std::time_t m_callCloseDT;
        std::string m_callID;
        std::string m_callStatus;
        unsigned    m_callerNumber;
        unsigned    m_operatorID;
    };

class CallCenter
{
public:
                CallCenter();
                ~CallCenter();
    bool        readRequest(const std::string &request);
    bool        readConfig(const std::string &fileName);
    bool        exportCDR();
    std::string dateToString(const time_t &src);
    void        run();

private:
    Config              m_config;
    std::vector<CDR>    m_CDRvec;
    std::queue<Call>    m_callsQueue;
};