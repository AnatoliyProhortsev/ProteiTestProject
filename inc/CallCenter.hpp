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
#include <map>

#include <httplib.h>
#include "../lib/json/json.hpp"

#include "Config.hpp"

struct CDR
{
    std::time_t  m_callReceiveDT;
    std::time_t  m_callAnswerDT;
    std::time_t  m_callCloseDT;
    std::time_t  m_callDuration;
    std::string  m_callID;
    std::string  m_callStatus;
    std::string  m_callerNumber;
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
    std::string m_Number;
    unsigned    m_awaitingTime;
    std::time_t m_receiveTime;
    std::string m_callID;
};

class CallCenter
{
public:
                CallCenter(const std::string &cfgName);

                CallCenter();

                ~CallCenter();

    bool        readConfig(const std::string &fileName);

    bool        exportCDR();

    void        start();

    FRIEND_TEST(test_call, just_call);
    FRIEND_TEST(test_call, call_duplication);
    FRIEND_TEST(test_call, wrong_cmd);
    FRIEND_TEST(test_call, wrong_number);
    FRIEND_TEST(config, wrong_init_cfg);
    FRIEND_TEST(config, valid_init_cfg);
    FRIEND_TEST(config, valid_cfg_in_run);

private:

    std::string dateToString(const time_t &src);

    std::string timeToString(const time_t &src);
    
    std::string dateToFileNameString(const time_t &src);

    std::string getRandomString();

    void        proceedCall_background(Call call,
                                        unsigned opID);

    void        distributeRequests_background();

    bool        statCollector_background();

    void        addCDREntry(
                    const std::time_t &receiveTime,
                    const std::time_t &answerTime,
                    const std::time_t &closeTime,
                    const std::time_t &callDuration,
                    const std::string &callID,
                    const std::string &status,
                    const std::string &number,
                    unsigned           opID,
                    bool               proceeded);

    bool        isUniqueID(const std::string &ID);

    std::mutex                  m_operatorsMutex;
    std::mutex                  m_callsMutex;
    std::mutex                  m_CDRMutex;
    std::mutex                  m_configMutex;
    std::mutex                  m_activeCallsMutex;

    Config                      m_config;

    std::vector<CDR>            m_CDRvec;
    std::vector<Operator>       m_Operators;
    std::vector<Call>           m_callsVec;
    std::vector<Call>           m_activeCallsVec;

    httplib::Server             m_server;

    #ifndef test
    std::shared_ptr
        <spdlog::logger>        m_FileLogger;
    #endif

    bool                        m_isWorking;
};