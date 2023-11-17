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

#include <httplib.h>
#include "../lib/httpparser/httprequestparser.h"
#include "../lib/httpparser/request.h"
#include "../lib/json/json.hpp"

#include "Config.hpp"

struct CDR
{
    std::time_t  m_callReceiveDT;
    std::time_t  m_callAnswerDT;
    std::time_t  m_callCloseDT;
    std::string  m_callID;
    std::string  m_callStatus;
    unsigned     m_callerNumber;
    unsigned     m_operatorID;
    bool         m_isCallProceeded;
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
    
    std::string dateToFileNameString(const time_t &src);

    std::string getRandomString();

    void        proceedCall_background(Call call,
                                        unsigned opID);

    void        distributeRequests_background();

    void        addCDREntry(
                    const std::time_t &receiveTime,
                    const std::time_t &answerTime,
                    const std::time_t &closeTime,
                    const std::string &callID,
                    const std::string &status,
                    unsigned           number,
                    unsigned           opID,
                    bool               proceeded);

    bool        isUniqueID(const std::string &ID);

    unsigned    readRequest(const std::string &request);

    std::mutex                  m_operatorsMutex;
    std::mutex                  m_callsMutex;
    std::mutex                  m_CDRMutex;
    std::mutex                  m_configMutex;
    Config                      m_config;
    std::vector<CDR>            m_CDRvec;
    std::vector<Operator>       m_Operators;
    std::vector<Call>           m_callsVec;
    bool                        m_isWorking;

    httplib::Server             m_server;
};