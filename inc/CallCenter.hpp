#include <queue>
#include <iostream>
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
//#include "CallDetailedRecord.hpp"
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
    unsigned    m_ID;
    bool        m_isBusy;
};

struct Call
{
    unsigned    m_Number;
    unsigned    m_awaitingTime;
    std::time_t m_receiveTime;
    std::string m_callID;
};

class CallCenter
{
public:
                CallCenter(const std::string &cfgName);

                ~CallCenter();

    bool        readConfig(const std::string &fileName);

    bool        exportCDR();

    void        run();

private:

    std::string dateToString(const time_t &src);

    std::string getRandomString();

    bool        proceedCall_background(Call call,
                                        unsigned operatorID);

    bool        distributeRequests_background();

    bool        isUniqueID(const std::string &ID);

    unsigned    readRequest(const std::string &request);

    std::mutex              m_mutex;
    Config                  m_config;
    std::vector<CDR>        m_CDRvec;
    std::vector<Operator>   m_Operators;
    std::vector<Call>       m_callsVec;
    bool                    m_isWorking;
};