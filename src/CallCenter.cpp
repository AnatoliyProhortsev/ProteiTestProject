#include "../inc/CallCenter.hpp"

CallCenter::CallCenter(const std::string &cfgName)
    :m_config(cfgName)
{
    for(unsigned i = 0; i < m_config.getOperatorsCount(); i++)
        m_Operators.push_back(Operator{i, false});

    std::time_t journalDate = time(0);
    #ifndef test
    m_FileLogger = spdlog::basic_logger_mt("fileLog", "../log/" + dateToFileNameString(journalDate));
    spdlog::get("fileLog")->info("CallCenter was inited with cfg: " + cfgName + ".");
    #endif

    srand(time(0));
}

CallCenter::CallCenter()
    :m_config()
{
    for(unsigned i = 0; i < m_config.getOperatorsCount(); i++)
        m_Operators.push_back(Operator{i, false});

    std::time_t journalDate = time(0);

    #ifndef test
    m_FileLogger = spdlog::basic_logger_mt("fileLog", "../log/" + dateToFileNameString(journalDate));
    spdlog::get("fileLog")->info("CallCenter inited with default cfg.");
    #endif

    srand(time(0));
}

CallCenter::~CallCenter()
{
    #ifndef test
    spdlog::info("Clearing all vectors.");
    #endif
    m_CDRvec.clear();
    m_callsVec.clear();
    m_Operators.clear();
    m_activeCallsVec.clear();
}

bool CallCenter::readConfig(const std::string &fileName)
{
    if(m_config.readConfigFile(fileName))
    {
        #ifndef test
        spdlog::info("Config file succesfully readed.");
        spdlog::get("fileLog")->info("Config file succesfully readed.");
        #endif
        return true;
    }
    #ifndef test
    spdlog::error("Can't read config file.");
    spdlog::get("fileLog")->error("Can't read config file.");
    #endif
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
    #ifndef test
    spdlog::info("Operator " + std::to_string(opID) + " started call " + call.m_callID);
    spdlog::get("fileLog")->info("Operator " + std::to_string(opID) + " started call " + call.m_callID);
    #endif
    
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
            #ifndef test
            m_activeCallsVec.erase(acIter);
            spdlog::debug("Call released.");
            spdlog::get("fileLog")->debug("Call released.");
            #endif
        }

        break;
    }
    m_activeCallsMutex.unlock();

    //Assign free status to current operator
    m_operatorsMutex.lock();
    m_Operators.at(opID).m_isBusy = false;
    m_operatorsMutex.unlock();
    #ifndef test
    spdlog::info("Operator " + std::to_string(opID) + " now free.");
    spdlog::get("fileLog")->info("Operator " + std::to_string(opID) + " now free.");
    #endif
}

void CallCenter::distributeRequests_background()
{
    #ifndef test
    spdlog::info("Distributor started.");
    spdlog::get("fileLog")->info("Distributor started.");
    #endif
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
                        #ifndef test
                        spdlog::debug("Deleting operator with ID: " + std::to_string((*opIter).m_ID));
                        spdlog::get("fileLog")->debug("Deleting operator with ID: " + std::to_string((*opIter).m_ID));
                        #endif
                        m_Operators.erase(opIter);
                    }
            }
        }else if(m_Operators.size() < m_config.getOperatorsCount())
        {
            for(unsigned i = m_Operators.size();
                i < m_config.getOperatorsCount();
                i++)
                {
                    #ifndef test
                    spdlog::debug("Adding operator with ID: " + std::to_string(i));
                    spdlog::get("fileLog")->debug("Adding operator with ID: " + std::to_string(i));
                    #endif
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
                    #ifndef test
                    spdlog::debug("Call "+ callToProceed.m_callID + " pushed to queue.");
                    spdlog::get("fileLog")->debug("Call "+ callToProceed.m_callID + " pushed to queue.");
                    #endif

                    m_activeCallsMutex.lock();
                    m_activeCallsVec.push_back(callToProceed);
                    m_activeCallsMutex.unlock();

                    activeCall.detach();

                    //Delete call from queue
                    m_callsVec.erase(m_callsVec.begin());
                    m_callsMutex.unlock();
                    #ifndef test
                    spdlog::debug("Call deleted from queue.");
                    spdlog::get("fileLog")->debug("Call deleted from queue.");
                    #endif
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
                        #ifndef test
                        spdlog::info("Call " + (*callsIter).m_callID + " dropped due to timeout.");
                        spdlog::get("fileLog")->info("Call " + (*callsIter).m_callID + " dropped due to timeout.");
                        #endif
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
    #ifndef test
    spdlog::info("Distributor awaiting unproceeded calls..");
    spdlog::get("fileLog")->info("Distributor awaiting unproceeded calls..");
    #endif
    while(!m_activeCallsVec.empty())
        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                500));
    #ifndef test
    spdlog::info("Distributor quitting.");
    spdlog::get("fileLog")->info("Distributor quitting.");
    #endif
}

void CallCenter::start()
{
    //TODO: Добавить чтение новой конфигурации

    m_isWorking = true;
    //Creating distributor thread(it will automatically run)
    std::thread distributor(&CallCenter::distributeRequests_background, this);
    //Creating background statistics collector 
    std::thread statCollector(&CallCenter::statCollector_background, this);

    //Add a shutdown POST query to server
    m_server.Post("/shutdown",
                [this]
                (const httplib::Request& req,
                 httplib::Response& res)
    {
        #ifndef test
        spdlog::info("Received shutdown cmd.");
        spdlog::get("fileLog")->info("Received shutdown cmd.");
        #endif
        m_isWorking = false;
        m_server.stop();
    });

    //Add a config change POST query to server
    m_server.Post("/config",
                [this]
                (const httplib::Request& req,
                 httplib::Response& res)
    {
        #ifndef test
        spdlog::info("Received config change cmd.");
        spdlog::get("fileLog")->info("Received config change cmd.");
        #endif
        std::string cfgFileName;

        if (req.has_param("name"))
        {
            cfgFileName = req.get_param_value("name");
            
            m_configMutex.lock();
            #ifndef test
            spdlog::debug("Trying to read new config file.");
            spdlog::get("fileLog")->debug("Trying to read new config file.");
            #endif
            if(!readConfig(cfgFileName))
            {
                #ifndef test
                spdlog::info("Invalid request: wrong file name");
                spdlog::get("fileLog")->info("Invalid request: wrong file name");
                #endif
                m_configMutex.unlock();
                res.set_content("Wrong file name\n", "text/plain");
                return;
            }
            else
            {
                #ifndef test
                spdlog::info("Config changed.");
                spdlog::get("fileLog")->info("Config changed.");
                #endif
                m_configMutex.unlock();
                res.set_content("OK\n", "text/plain");
                return;
            }
        }else
        {
            #ifndef test
            spdlog::info("Wrong request.");
            spdlog::get("fileLog")->info("Wrong request.");
            #endif
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
            #ifndef test
            spdlog::info("Received call cmd with param: " + number);
            spdlog::get("fileLog")->info("Received call cmd with param: " + number);
            #endif
            if(number.length() == 11)
            {
                for (char const& c : number) 
                {
                    if (std::isdigit(c) == 0)
                    {
                        #ifndef test
                        spdlog::info("Invalid request: not a number.");
                        spdlog::get("fileLog")->info("Invalid request: not a number.");
                        #endif
                        res.set_content("Not a number\n", "text/plain");
                        return;
                    }
                }
            }else
            {
                #ifndef test
                spdlog::error("Invalid request: not a valid number.");
                spdlog::get("fileLog")->error("Invalid request: not a valid number.");
                #endif
                res.set_content("Not a valid number\n", "text/plain");
                return;
            }
        }else
        {
            #ifndef test
            spdlog::error("Wrong request.");
            spdlog::get("fileLog")->error("Wrong request.");
            #endif
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
                #ifndef test
                spdlog::info("Call duplication.");
                spdlog::get("fileLog")->info("Call duplication.");
                #endif
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
                #ifndef test
                spdlog::info("Call duplication.");
                spdlog::get("fileLog")->info("Call duplication.");
                #endif
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
                #ifndef test
                spdlog::info("System is overloaded to proceed request.");
                spdlog::get("fileLog")->info("System is overloaded to proceed request.");
                #endif
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
                #ifndef test
                spdlog::info("Sended call ID " + callID + " to client.");
                spdlog::get("fileLog")->info("Sended call ID " + callID + " to client.");
                #endif
            }
        m_callsMutex.unlock();
        return;
    });

    //Start listening to queries
    #ifndef test
    spdlog::info("Server started listening incoming queries.");
    spdlog::get("fileLog")->info("Server started listening incoming queries.");
    #endif
    m_server.listen("localhost", 8080);

    //Awaiting to distributor to finish
    #ifndef test
    spdlog::info("Server awaiting to distributor shutdown...");
    spdlog::get("fileLog")->info("Server awaiting to distributor shutdown...");
    #endif
    distributor.join();
    #ifndef test
    spdlog::info("Server awaiting to stat collector shutdown...");
    spdlog::get("fileLog")->info("Server awaiting to stat collector shutdown...");
    #endif
    statCollector.join();
    #ifndef test
    spdlog::info("Server shutting down");
    spdlog::get("fileLog")->info("Server shutting down");
    #endif
}

bool CallCenter::exportCDR()
{
    #ifndef test
    spdlog::info("Started to exporting CDR journal.");
    spdlog::get("fileLog")->info("Started to exporting CDR journal.");
    #endif
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
        #ifndef test
        spdlog::error("Error to export CDR journal: can't open output file");
        spdlog::get("fileLog")->error("Error to export CDR journal: can't open output file");
        #endif
        return false;
    }

    //Output CDR journal
    outputFile << std::setw(4) << cdrJson;
    outputFile.close();
    #ifndef test
    spdlog::info("CDR journal exported.");
    spdlog::get("fileLog")->info("CDR journal exported.");
    #endif
    return true;
}

bool CallCenter::statCollector_background()
{
    json journal = json::array();
    json journalEntry;

    unsigned minAwaiting = UINT_MAX,
            avAwating = 0,
            maxAwaiting = 0,
            minSize = UINT_MAX,
            avSize = 0,
            maxSize = 0,
            minBusyCount = UINT_MAX,
            avBusyCount = 0,
            maxBusyCount = 0;

    unsigned curCdrEntry = 0;

    unsigned awaitingSum = 0,
            sizeSum = 0,
            busyCountSum = 0;

    unsigned count = 0;

    std::time_t answer, receive;

    while(m_isWorking)
    {
        journalEntry.clear();

        count = m_CDRvec.size() - curCdrEntry;
        for(unsigned count = 0; count < 10; count++)
        {
            for(curCdrEntry; curCdrEntry != m_CDRvec.size(); curCdrEntry++)
            {
                if(m_CDRvec[curCdrEntry].m_isCallProceeded)
                {
                    answer = m_CDRvec[curCdrEntry].m_callAnswerDT;
                    receive = m_CDRvec[curCdrEntry].m_callReceiveDT;

                    double time_diff = difftime(answer, receive);

                    if(time_diff < minAwaiting)
                        minAwaiting = time_diff;

                    awaitingSum+=(unsigned)time_diff;

                    if(time_diff > maxAwaiting)
                        maxAwaiting = time_diff;
                }
            }
            if(m_callsVec.size() < minSize)
                        minSize = m_callsVec.size();
                    
            if(m_activeCallsVec.size() < minBusyCount)
                minBusyCount = m_activeCallsVec.size();

            sizeSum += m_callsVec.size();
            busyCountSum += m_activeCallsVec.size();

            if(m_callsVec.size() > maxSize)
                        maxSize = m_callsVec.size();

            if(m_activeCallsVec.size() > maxBusyCount)
                maxBusyCount = m_activeCallsVec.size();

            std::this_thread::sleep_for(
            std::chrono::milliseconds(
                1000));
        }
        avSize = sizeSum/10;

        if(count !=0)
            avAwating = awaitingSum/count;
        else
            avAwating = 0;

        avBusyCount = busyCountSum/10;

        if(minAwaiting == UINT_MAX)
            minAwaiting = 0;

        journalEntry["minimum awaitingTime"] = minAwaiting;
        journalEntry["average awating time"] = avAwating;
        journalEntry["maximum awating time"] = maxAwaiting;
        journalEntry["minimum queue size"] = minSize;
        journalEntry["average queue size"] = avSize;
        journalEntry["maximum queue size"] = maxSize;
        journalEntry["minimum busy operators"] = minBusyCount;
        journalEntry["average busy operators"] = avBusyCount;
        journalEntry["maximum busy operators"] = maxBusyCount;
        journalEntry["timestamp"] = timeToString(time(0));
        
        journal.emplace_back(std::move(journalEntry));

        #ifndef test
        spdlog::info("Stats perfomed.");
        spdlog::get("fileLog")->info("Stats perfomed.");
        #endif

        minAwaiting = UINT_MAX,
        avAwating = 0;
        maxAwaiting = 0;
        minSize = UINT_MAX;
        avSize = 0;
        maxSize = 0;
        minBusyCount = UINT_MAX;
        avBusyCount = 0;
        maxBusyCount = 0;

        awaitingSum = 0;
        sizeSum = 0;
        busyCountSum = 0;

        count = 0;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(
                10000));
    }

    std::time_t journalDate = time(0);
    std::ofstream outputFile(
        "../stats/" + dateToFileNameString(journalDate),
        std::ofstream::out);

    if(!outputFile)
    {
        #ifndef test
        spdlog::error("Error to export stats journal: can't open output file");
        spdlog::get("fileLog")->error("Error to export stats journal: can't open output file");
        #endif
        return false;
    }

    //Output stats journal
    outputFile << std::setw(4) << journal;
    outputFile.close();
    #ifndef test
    spdlog::info("Stats journal exported.");
    spdlog::get("fileLog")->info("Stats journal exported.");
    #endif
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