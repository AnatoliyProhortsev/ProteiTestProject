#include "../inc/CallCenter.hpp"

CallCenter::CallCenter(const std::string &cfgName)
    :m_config(cfgName)
{
    for(unsigned i = 0; i < m_config.getOperatorsCount(); i++)
        m_Operators.push_back(Operator{i, false});

    srand(time(0));
}

CallCenter::CallCenter()
    :m_config()
{
    for(unsigned i = 0; i < m_config.getOperatorsCount(); i++)
        m_Operators.push_back(Operator{i, false});

    srand(time(0));
}

CallCenter::~CallCenter()
{
    spdlog::info("Clearing all vectors.");
    m_CDRvec.clear();
    m_callsVec.clear();
    m_Operators.clear();
    m_activeCallsVec.clear();
}

bool CallCenter::readConfig(const std::string &fileName)
{
    if(m_config.readConfigFile(fileName))
    {
        spdlog::info("Config file succesfully readed.");
        return true;
    }
    spdlog::error("Can't read config file.");
    return false;
}

void CallCenter::proceedCall_background(Call call, unsigned opID)
{
    unsigned rMin       = m_config.getRmin();
    unsigned rMax       = m_config.getRmax();
    unsigned procTime   = m_config.getProcessingTime();

    //Simulation of answering
    std::this_thread::sleep_for(std::chrono::milliseconds(
        rand() %
        (rMax - rMin + 1)
        + rMin
    ));

    //Simulation of call
    spdlog::info("Operator " + std::to_string(opID) + " started call " + call.m_callID);
    std::time_t answerTime = time(0);
    std::chrono::milliseconds timespan(procTime);
    std::this_thread::sleep_for(timespan);

    //Add a CDR entry for succeded call
    std::time_t closeTime = time(0);
    m_CDRMutex.lock();
    addCDREntry(
                call.m_receiveTime,
                answerTime,
                closeTime,
                closeTime - answerTime,
                call.m_callID,
                "OK",
                call.m_Number,
                opID,
                true);      
    m_CDRMutex.unlock();

    //Delete call from active calls
    m_activeCallsMutex.lock();
    for(auto acIter = m_activeCallsVec.begin();
            acIter!= m_activeCallsVec.end();
            acIter++)
    {
        if((*acIter).m_Number == call.m_Number)
        {
            m_activeCallsVec.erase(acIter);
            spdlog::debug("Call released.");
        }

        break;
    }
    m_activeCallsMutex.unlock();

    //Assign free status to current operator
    m_operatorsMutex.lock();
    m_Operators.at(opID).m_isBusy = false;
    m_operatorsMutex.unlock();
    spdlog::info("Operator " + std::to_string(opID) + " now free.");
}

void CallCenter::distributeRequests_background()
{
    spdlog::info("Distributor started.");
    unsigned rMax = m_config.getRmax();
    unsigned rMin = m_config.getRmin();

    while (m_isWorking)
    {
        m_operatorsMutex.lock();
        if(m_Operators.size() > m_config.getOperatorsCount())
        {
            for(auto opIter = m_Operators.begin();
                    opIter != m_Operators.end();
                    opIter++)
            {
                if(m_Operators.size() > m_config.getOperatorsCount())
                    if(!(*opIter).m_isBusy)
                    {
                        spdlog::debug("Deleting operator with ID: " + std::to_string((*opIter).m_ID));
                        m_Operators.erase(opIter);
                    }
            }
        }else if(m_Operators.size() < m_config.getOperatorsCount())
        {
            for(unsigned i = m_Operators.size();
                i < m_config.getOperatorsCount();
                i++)
                {
                    spdlog::debug("Adding operator with ID: " + std::to_string(i));
                    m_Operators.push_back(Operator{i, false});
                }
                    
        }
        m_operatorsMutex.unlock();

        while (!m_callsVec.empty())
        {
            m_operatorsMutex.lock();
            for(auto opIter = m_Operators.begin();
                    opIter != m_Operators.end();
                    opIter++)
            {
                if(!(*opIter).m_isBusy)
                {
                    //Create and detach a thread for call
                    m_callsMutex.lock();
                    (*opIter).m_isBusy = true;

                    Call callToProceed = m_callsVec.front();
                    std::thread activeCall(
                        &CallCenter::proceedCall_background,
                        this,
                        callToProceed,
                        (*opIter).m_ID
                    );
                    spdlog::debug("Call "+ callToProceed.m_callID + " pushed to queue.");

                    m_activeCallsMutex.lock();
                    m_activeCallsVec.push_back(callToProceed);
                    m_activeCallsMutex.unlock();

                    activeCall.detach();

                    //Delete call from queue
                    m_callsVec.erase(m_callsVec.begin());
                    m_callsMutex.unlock();
                    spdlog::debug("Call deleted from queue.");
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
                        //Add a CDR entry with timeout release
                        addCDREntry(
                            (*callsIter).m_receiveTime,
                            0,
                            time(0),
                            0,
                            (*callsIter).m_callID,
                            "timeout",
                            (*callsIter).m_Number,
                            0,
                            false);
                        //Delete call from queue
                        callsIter = m_callsVec.erase(callsIter);
                        spdlog::info("Call " + (*callsIter).m_callID + " dropped due to timeout.");
                    }else
                    {
                        //increase query awating time based on distributor "clock"
                        (*callsIter).m_awaitingTime+=250;
                        callsIter++;
                    }       
            m_callsMutex.unlock();

            //Distributor clock
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

    //Await for all active calls before shutting down
    spdlog::info("Distributor awaiting unproceeded calls..");
    while(!m_activeCallsVec.empty())
        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                500));
    spdlog::info("Distributor quitting.");
}

void CallCenter::start()
{
    //TODO: Добавить чтение новой конфигурации

    m_isWorking = true;
    //Creating distributor thread(it will automatically run)
    std::thread distributor(&CallCenter::distributeRequests_background, this);

    //Add a shutdown POST query to server
    m_server.Post("/shutdown",
                [this]
                (const httplib::Request& req,
                 httplib::Response& res)
    {
        spdlog::info("Received shutdown cmd.");
        m_isWorking = false;
        m_server.stop();
    });

    //Add a config change POST query to server
    m_server.Post("/config",
                [this]
                (const httplib::Request& req,
                 httplib::Response& res)
    {
        spdlog::info("Received config change cmd.");
        std::string cfgFileName;

        if (req.has_param("name"))
        {
            cfgFileName = req.get_param_value("name");
            
            m_configMutex.lock();
            spdlog::debug("Trying to read new config file.");
            if(!readConfig(cfgFileName))
            {
                spdlog::info("Invalid request: wrong file name");
                m_configMutex.unlock();
                res.set_content("Wrong file name\n", "text/plain");
                return;
            }
            else
            {
                spdlog::info("Config changed.");
                m_configMutex.unlock();
                res.set_content("OK\n", "text/plain");
                return;
            }
        }else
        {
            spdlog::info("Wrong request.");
            res.set_content("Wrong request\n", "text/plain");
            return;
        }
    });

    //Add a call POST query to server
    m_server.Post("/call", [this](const httplib::Request& req, httplib::Response& res)
    {
        std::string number;

        //Query parameter validation(i.e number validation)
        if (req.has_param("number"))
        {
            number = req.get_param_value("number");
            spdlog::info("Received call cmd with param: " + number);
            if(number.length() == 11)
            {
                for (char const& c : number) 
                {
                    if (std::isdigit(c) == 0)
                    {
                        spdlog::info("Invalid request: not a number.");
                        res.set_content("Not a number\n", "text/plain");
                        return;
                    }
                }
            }else
            {
                spdlog::error("Invalid request: not a valid number.");
                res.set_content("Not a valid number\n", "text/plain");
                return;
            }
        }else
        {
            spdlog::error("Wrong request.");
            res.set_content("Wrong request\n", "text/plain");
            return;
        }

        //Checking call duplication in queue
        m_callsMutex.lock();
        for(auto callsIter = m_callsVec.begin();
                callsIter != m_callsVec.end();
                callsIter++)
        {
            if((*callsIter).m_Number.compare(number) == 0)
            {
                spdlog::info("Call duplication.");
                res.set_content("Call already in queue\n", "text/plain");
                m_callsMutex.unlock();
                return;
            }
        }
        m_callsMutex.unlock();

        //Checking call duplication in active calls
        m_activeCallsMutex.lock();
        for(auto acIter = m_activeCallsVec.begin();
                acIter!= m_activeCallsVec.end();
                acIter++)
        {
            if((*acIter).m_Number.compare(number) == 0)
            {
                spdlog::info("Call duplication.");
                res.set_content("Call already proceeding\n", "text/plain");
                m_activeCallsMutex.unlock();
                return;
            }
        }
        m_activeCallsMutex.unlock();


        unsigned queueSize = m_config.getQueueSize();

        m_callsMutex.lock();
            if(m_callsVec.size() >= queueSize)
            {
                //Queue overload case
                spdlog::info("System is overloaded to proceed request.");
                res.set_content("Overload.\n", "text/plain");
            }
            else
            {
                //Normally pushing call to queue
                std::time_t callReceiveDT = time(0);
                std::string callID = getRandomString();

                m_callsVec.push_back(Call{
                                number,
                                0,
                                callReceiveDT,
                                callID});
                res.set_content("Call ID: " + callID + '\n', "text/plain");
                spdlog::info("Sended call ID " + callID + " to client.");     
            }
        m_callsMutex.unlock();
        return;
    });

    //Start listening to queries
    spdlog::info("Server started listening incoming queries.");
    m_server.listen("localhost", 8080);

    //Awaiting to distributor to finish
    spdlog::info("Server awaiting to distributor shutdown");
    distributor.join();
    spdlog::info("Server shutting down");
}

bool CallCenter::exportCDR()
{
    spdlog::info("Started to exporting CDR journal.");
    if(m_CDRvec.empty())
        return false;

    //CDR journal represented as json array
    json cdrJson = json::array();
    
    for(auto iter = m_CDRvec.begin(); iter != m_CDRvec.end(); iter++)
    {
        //Create an json object and fill it with values
        json cdrEntry;
        cdrEntry.clear();

        if((*iter).m_isCallProceeded)
        {
            cdrEntry["CallReceiveDT"] = dateToString((*iter).m_callReceiveDT);
            cdrEntry["CallAnswerDT"] = dateToString((*iter).m_callAnswerDT);
            cdrEntry["CallCloseDT"] = dateToString((*iter).m_callCloseDT);
            cdrEntry["CallDuration"] = timeToString((*iter).m_callDuration);
            cdrEntry["CallID"] = (*iter).m_callID;
            cdrEntry["CallStatus"] = (*iter).m_callStatus;
            cdrEntry["CallerNumber"] = (*iter).m_callerNumber;
            cdrEntry["OperatorID"] = (*iter).m_operatorID;
        }
        else
        {
            cdrEntry["CallReceiveDT"] = dateToString((*iter).m_callReceiveDT);
            cdrEntry["CallCloseDT"] = dateToString((*iter).m_callCloseDT);
            cdrEntry["CallID"] = (*iter).m_callID;
            cdrEntry["CallStatus"] = (*iter).m_callStatus;
            cdrEntry["CallerNumber"] = (*iter).m_callerNumber;
        }
        //Move object to json array
        cdrJson.emplace_back(std::move(cdrEntry));
    }

    //Create CDR journal with DT as file name
    std::time_t journalDate = time(0);
    std::ofstream outputFile(
        "../cdr/" + dateToFileNameString(journalDate),
        std::ofstream::out);

    if(!outputFile)
    {
        spdlog::error("Error to export CDR journal: can't open output file");
        return false;
    }

    //Output CDR journal
    outputFile << std::setw(4) << cdrJson;
    outputFile.close();
    spdlog::info("CDR journal exported.");
    return true;
}

std::string CallCenter::dateToString(const time_t &src)
{
    //Cast time_t to string
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

std::string CallCenter::timeToString(const time_t &src)
{
    //Cast time_t to string but only hh:mm::ss
    tm *ltm = localtime(&src);
    std::stringstream dateString;
    dateString
         << ltm->tm_hour - 3
         << ':'
         << ltm->tm_min
         << ':'
         << ltm->tm_sec;
    return dateString.str();
}

std::string CallCenter::dateToFileNameString(const time_t &src)
{
    //Cast time_t to string but without special characters
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
    //Create call ID from array of chars
    const int max_len = 10;
    std::string valid_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 g(rd());
    std::string rand_str;

    //call ID can't repeats on at least current CDR journal
    do
    {
        std::shuffle(valid_chars.begin(), valid_chars.end(), g);
        rand_str = std::string(valid_chars.begin(), valid_chars.begin() + max_len);
    } while(!isUniqueID(rand_str));

    return rand_str;
}

bool CallCenter::isUniqueID(const std::string &ID)
{
    //Look in CDR journal for our new call ID
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
                    const std::time_t &callDuration,
                    const std::string &callID,
                    const std::string &status,
                    const std::string &number,
                    unsigned           opID,
                    bool               proceeded)
{
    //Function for better code
    m_CDRvec.push_back(
                    CDR{
                        receiveTime,
                        answerTime,
                        closeTime,
                        callDuration,
                        callID,
                        status,
                        number,
                        opID,
                        proceeded
                    });
}