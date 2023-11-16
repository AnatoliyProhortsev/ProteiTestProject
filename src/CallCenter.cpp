#include "../inc/CallCenter.hpp"

CallCenter::CallCenter(const std::string &cfgName)
    :m_config(cfgName)
{
    for(unsigned i = 0; i < m_config.getOperatorsCount(); i++)
        m_Operators.push_back(Operator{i, false});

    srand(time(0));
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

void CallCenter::proceedCall_background(Call call, unsigned opID)
{
    m_configMutex.lock();
    unsigned rMin       = m_config.getRmin();
    unsigned rMax       = m_config.getRmax();
    unsigned procTime   = m_config.getProcessingTime();
    m_configMutex.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(
        rand() %
        (rMax - rMin + 1)
        + rMin
    ));

    std::time_t answerTime = time(0);
    std::chrono::milliseconds timespan(procTime);
    std::this_thread::sleep_for(timespan);

    m_CDRMutex.lock();
    addCDREntry(
        call.m_receiveTime,
                        answerTime,
                        time(0),
                        call.m_callID,
                        "OK",
                        call.m_Number,
                        opID);      
    m_CDRMutex.unlock();

    m_operatorsMutex.lock();
    m_Operators.at(opID).m_isBusy = false;
    m_operatorsMutex.unlock();
}

void CallCenter::distributeRequests_background()
{
    unsigned rMax = m_config.getRmax();
    unsigned rMin = m_config.getRmin();

    while (m_isWorking)
    {
        while (!m_callsVec.empty())
        {
            m_operatorsMutex.lock();
            for(auto opIter = m_Operators.begin();
                    opIter != m_Operators.end();
                    opIter++)
            {
                if(!(*opIter).m_isBusy)
                {
                    m_callsMutex.lock();
                    (*opIter).m_isBusy = true;

                    std::thread activeCall(
                        &CallCenter::proceedCall_background,
                        this,
                        std::move(m_callsVec.front()),
                        (*opIter).m_ID
                    );
                    activeCall.detach();
                    m_callsVec.erase(m_callsVec.begin());

                    m_callsMutex.unlock();
                    break;
                }
            }
            m_operatorsMutex.unlock();

            m_callsMutex.lock();
            auto callsIter = m_callsVec.begin();
            if(!m_callsVec.empty())
                while(callsIter!= m_callsVec.end())
                    if((*callsIter).m_awaitingTime >= rMax - rMin)
                    {
                        //timeout
                        addCDREntry(
                            (*callsIter).m_receiveTime,
                            0,
                            time(0),
                            (*callsIter).m_callID,
                            "timeout",
                            (*callsIter).m_Number,
                            0);
                        callsIter = m_callsVec.erase(callsIter);
                        
                    }else
                    {
                        (*callsIter).m_awaitingTime+=250;
                        callsIter++;
                    }
                        
            m_callsMutex.unlock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(m_config.getProcessingTime()));
}

void CallCenter::run()
{
    //TODO: Нормальное чтение запросов
    //TODO: Убирать поля у записей несостоявшихся звонков
    //TODO: Добавить длительность звонков в CDR
    unsigned queueSize = m_config.getQueueSize();

    std::string query;
    std::cout<<"Enter a http request: ";
    std::cin>>query;

    m_isWorking = true;

    std::thread distributor(&CallCenter::distributeRequests_background, this);

    while (query != "exit")
    {
        std::time_t callReceiveDT = time(0);
        std::string callID = getRandomString();
        unsigned callerNum = atoi(query.c_str());
        std::time_t callCloseDT;

        if(callerNum != 0)
        {
            m_callsMutex.lock();
            if(m_callsVec.size() == queueSize)
                std::cout<<"Overload.\n";
            else
                m_callsVec.push_back(Call{
                                callerNum,
                                0,
                                callReceiveDT,
                                callID});

            m_callsMutex.unlock();
        }else
        {
            std::cout<<"Bad Request.\n";
            //Неверно задан номер
        }

        std::cout<<"Enter a http request: ";
        std::cin>>query;
    }
    m_isWorking = false;
    distributor.join();
}

bool CallCenter::exportCDR()
{
    if(m_CDRvec.empty())
        return false;

    json cdrJson = json::array();
    
    for(auto iter = m_CDRvec.begin(); iter != m_CDRvec.end(); iter++)
    {
        json cdrEntry;
        cdrEntry["CallReceiveDT"] = dateToString((*iter).m_callReceiveDT);
        cdrEntry["CallAnswerDT"] = dateToString((*iter).m_callAnswerDT);
        cdrEntry["CallCloseDT"] = dateToString((*iter).m_callCloseDT);
        cdrEntry["CallID"] = (*iter).m_callID;
        cdrEntry["CallStatus"] = (*iter).m_callStatus;
        cdrEntry["CallerNumber"] = (*iter).m_callerNumber;
        cdrEntry["OperatorID"] = (*iter).m_operatorID;
        cdrJson.emplace_back(std::move(cdrEntry));
    }

    std::time_t journalDate = time(0);

    std::ofstream outputFile(
        "../cdr/" + dateToFileNameString(journalDate),
        std::ofstream::out);

    if(!outputFile)
        return false;

    outputFile << std::setw(4) << cdrJson;
    outputFile.close();
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
         << ltm->tm_hour
         << ':'
         << ltm->tm_min
         << ':'
         << ltm->tm_sec;
    return dateString.str();
}

std::string CallCenter::dateToFileNameString(const time_t &src)
{
    tm *ltm = localtime(&src);
    std::stringstream dateString;
    dateString << ltm->tm_mday
         << '_'
         << 1 + ltm->tm_mon
         << '_'
         << 1900 + ltm->tm_year
         << '_'
         << ltm->tm_hour
         << '_'
         << ltm->tm_min
         << '_'
         << ltm->tm_sec;
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

    m_CDRMutex.lock();
    for(auto idIter = m_CDRvec.begin();
            idIter != m_CDRvec.end();
            idIter++)
    {
        if((*idIter).m_callID == ID)
            return false;
    }
    m_CDRMutex.unlock();
    return true;
}

void CallCenter::addCDREntry(
                    const std::time_t &receiveTime,
                    const std::time_t &answerTime,
                    const std::time_t &closeTime,
                    const std::string &callID,
                    const std::string &status,
                    unsigned           number,
                    unsigned           opID)
{
    m_CDRvec.push_back(
                    CDR{
                        receiveTime,
                        answerTime,
                        closeTime,
                        callID,
                        status,
                        number,
                        opID
                    });
}