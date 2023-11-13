#include "../inc/CallCenter.hpp"

CallCenter::CallCenter()
    :m_config()
{
}

bool CallCenter::readConfig(const std::string &fileName)
{
    if(m_config.readConfigFile(fileName))
    {
        //success
    }else
    {
        //no config
    }
}

unsigned CallCenter::readRequest(const std::string &request)
{
    std::string tempStr = request;
    if(request.find("CALL"))
    {
        tempStr.replace(request.find("CALL"), 4, "");
        tempStr.erase(std::remove_if
            (tempStr.begin(), tempStr.end(), isspace), tempStr.end());
        if(tempStr.length() == 11)
        {
            return atoi(tempStr.c_str());
        }
        else
        {
            //Не удалось обработать номер
            return 0;
        }
    }
}

std::time_t proceedCall(const unsigned processingTime)
{
    std::chrono::milliseconds timespan(processingTime);
    std::this_thread::sleep_for(timespan);
    return std::time(0);
}

CDR proceedRequest(const std::time_t &callReceiveDT,
                    const std::string &callID,
                    unsigned operatorID,
                    unsigned processingTime,
                    unsigned rMin,
                    unsigned rMax)
{
    //TODO: нужно переписать логику
    //run() будет каждый интервал проверять массив заявок, смотреть у кого истекло ожидание
    //Создать структуру заявка
    //И наверное уже ассинхронно вызывать proceedCall(), если заявка выбрана для обработки
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
    } while(std::find(
        m_CDRvec.begin(),
        m_CDRvec.end(),
        rand_str)
        != m_CDRvec.end());

    return rand_str;
}

void CallCenter::run()
{
    std::string query;

    std::cout<<"Enter a http request: ";
    std::cin>>query;

    while (query != "exit")
    {
        std::time_t callReceiveDT = time(0);
        std::string callID = getRandomString();
        unsigned callerNum = readRequest(query);
        std::time_t callCloseDT;
        bool haveFreeOp = false;

        if(callerNum != 0)
        {
            
            for(auto opIter = m_Operators.begin();
                opIter != m_Operators.end(); opIter++)
            {
                if(!(*opIter).m_isBusy)
                {
                    haveFreeOp = true;
                    // auto runner = 
                    //     std::async(
                    //         std::launch::async,
                    //         proceedRequest,
                    //         callReceiveDT,
                    //         callID,
                    //         (*opIter).m_ID,
                    //         m_config.getProcessingTime(),
                    //         m_config.getRmin,
                    //         m_config.getRmax);
                }
            }
            if(!haveFreeOp)
            {
                m_callsQueue.push(callerNum);
            }
            //Формировать CDR
        }else
        {
            //Неверно задан номер
        }
    }
    
}

bool CallCenter::exportCDR()
{
    if(m_CDRvec.empty())
        return false;

    std::time_t journalDate = time(0);
    json cdrJson = json::array();

    for(auto iter {m_CDRvec.begin()}; iter != m_CDRvec.end(); iter++)
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

std::string dateToString(const time_t &src)
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