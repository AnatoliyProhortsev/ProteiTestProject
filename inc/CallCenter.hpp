#include <queue>
#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <vector>
#include <future>
#include <thread>
#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>

#include "../lib/httpparser/httprequestparser.h"
#include "../lib/httpparser/request.h"
#include "../lib/json/json.hpp"

//#include "Call.hpp"
#include "CallDetailedRecord.hpp"
#include "Config.hpp"
//#include "Operator.hpp"

struct CDR
{
    std::time_t  m_callReceiveDT;
    std::time_t  m_callAnswerDT;
    std::time_t  m_callCloseDT;
    std::string  m_callID;
    std::string  m_callStatus;
    unsigned     m_callerNumber;
    unsigned     m_operatorID;
};

struct Operator
{
    unsigned m_ID;
    bool m_isBusy;
};

class CallCenter
{
public:
                CallCenter();
                ~CallCenter();
    unsigned    readRequest(const std::string &request);
    bool        readConfig(const std::string &fileName);
    bool        exportCDR();
    std::string dateToString(const time_t &src);
    std::string getRandomString();
    //std::time_t proceedCall(const unsigned processingTime);
    void        run();

private:
    Config                  m_config;
    std::vector<CDR>        m_CDRvec;
    std::vector<Operator>   m_Operators;
    std::queue<unsigned>    m_callsQueue;
};