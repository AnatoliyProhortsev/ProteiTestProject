#include "../inc/CallCenter.hpp"

CallCenter::CallCenter(const std::string &cfgName)
    :m_config(cfgName)
{
    for(unsigned i = 0; i < m_config.getOperatorsCount(); i++)
        m_Operators.push_back(Operator{i, false});
}

CallCenter::~CallCenter()
{
    m_CDRvec.clear();
    m_callsVec.clear();
    m_Operators.clear();
}

bool CallCenter::readConfig(const std::string &fileName)
{
    if(m_config.readConfigFile(fileName))
    {
        return true;
    }
    return false;
}

unsigned CallCenter::readRequest(const std::string &request)
{
    std::string tempStr = request;

    if(request.find("CALL"))
    {
        tempStr.replace(request.find("CALL"), 4, "");
        tempStr.erase(std::remove_if
                        (tempStr.begin(),
                        tempStr.end(),
                        isspace),
                        tempStr.end());
        if(tempStr.length() == 11)
            return atoi(tempStr.c_str());
        else
            return 0;   
    }

    return 0;
}

bool CallCenter::proceedCall_background
                    (Call call,
                    unsigned operatorID)
{
    std::time_t answerTime = time(0);
    std::chrono::milliseconds timespan(m_config.getProcessingTime());
    std::this_thread::sleep_for(timespan);
    m_CDRvec.push_back(
                    CDR{
                        call.m_receiveTime,
                        answerTime,
                        time(0),
                        call.m_callID,
                        "OK",
                        call.m_Number,
                        operatorID
                    });

    return true;
}

bool CallCenter::distributeRequests_background()
{
    std::time_t curTime;
    std::queue<std::future<bool>> activeCalls;

    while (m_isWorking)
    {
        while (!m_callsVec.empty())
        {
            for(auto opIter = m_Operators.begin();
                    opIter != m_Operators.end();
                    opIter++)
            {
                if(!(*opIter).m_isBusy)
                {
                    m_mutex.lock();
                    (*opIter).m_isBusy = true;
                    activeCalls.push(
                        std::async(
                            std::launch::async,
                            &CallCenter::proceedCall_background,
                            this,
                            m_callsVec.front(),
                            (*opIter).m_ID));
                    
                    m_callsVec.erase(m_callsVec.begin());
                    m_mutex.unlock();
                }
            }
            m_mutex.lock();
            for(auto callsIter = m_callsVec.begin();
                    callsIter != m_callsVec.end();
                    callsIter++)
            {
                (*callsIter).m_awaitingTime+=500;
            }
            m_mutex.unlock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return true;
}

void CallCenter::run()
{
    std::string query;
    std::cout<<"Enter a http request: ";
    std::cin>>query;

    m_isWorking = true;

    auto distributorHandle = std::async(
                                std::launch::async,
                                &CallCenter::distributeRequests_background,
                                this);

    while (query != "exit")
    {
        std::time_t callReceiveDT = time(0);
        std::string callID = getRandomString();
        unsigned callerNum = readRequest(query);
        std::time_t callCloseDT;

        if(callerNum != 0)
        {
            m_mutex.lock();

            if(m_callsVec.size() == m_config.getQueueSize())
                std::cout<<"Overload.\n";
            else
                m_callsVec.push_back(Call{
                                callerNum,
                                0,
                                callReceiveDT,
                                callID});
                std::cout<<"pushed\n";

            m_mutex.unlock();
        }else
        {
            std::cout<<"Bad Request.\n";
            //Неверно задан номер
        }

        std::cout<<"Enter a http request: ";
        std::cin>>query;
    }
    m_isWorking = false;
}

bool CallCenter::exportCDR()
{
    if(m_CDRvec.empty())
        return false;

    std::time_t journalDate = time(0);
    json cdrJson = json::array();

    for(auto iter = m_CDRvec.begin(); iter != m_CDRvec.end(); iter++)
    {
        cdrJson.emplace_back(
            json::array({
            {"CallReceiveDT" ,  dateToString((*iter).m_callReceiveDT)},
            {"CallAnswerDT" ,   dateToString((*iter).m_callAnswerDT)},
            {"CallCloseDT" ,    dateToString((*iter).m_callCloseDT)},
            {"CallID" ,         (*iter).m_callID},
            {"CallStatus" ,     (*iter).m_callStatus},
            {"CallerNumber" ,   (*iter).m_callerNumber},
            {"OperatorID" ,     (*iter).m_operatorID}
            }));
    }

    std::ofstream outputFile("../cdr/" + dateToString(journalDate));
    if(!outputFile)
        return false;

    outputFile << std::setw(4) << cdrJson;
    return true;
}

std::string CallCenter::dateToString(const time_t &src)
{
    tm *ltm = localtime(&src);
    std::stringstream dateString;
    dateString << ltm->tm_mday
         << '/'
         << 1 + ltm->tm_mon
         << '/'
         << 1900 + ltm->tm_year
         << ' '
         << 1 + ltm->tm_hour
         << ':'
         << 1 + ltm->tm_min
         << ':'
         << 1 + ltm->tm_sec;
    return dateString.str();
}

std::string CallCenter::getRandomString()
{
    const int max_len = 10;
    std::string valid_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 g(rd());
    std::string rand_str;
    do
    {
        std::shuffle(valid_chars.begin(), valid_chars.end(), g);
        rand_str = std::string(valid_chars.begin(), valid_chars.begin() + max_len);
    } while(!isUniqueID(rand_str));

    return rand_str;
}

bool CallCenter::isUniqueID(const std::string &ID)
{
    if(m_CDRvec.empty())
        return true;

    m_mutex.lock();
    for(auto idIter = m_CDRvec.begin();
            idIter != m_CDRvec.end();
            idIter++)
    {
        if((*idIter).m_callID == ID)
            return false;
    }
    m_mutex.unlock();
    return true;
}