#include "../inc/CallCenter.hpp"

CallCenter::CallCenter()
    :m_config()
{
    Operator::m_freeOperatorsCount = m_config.getOperatorsCount();
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

bool CallCenter::readRequest(const std::string &request)
{

}

void CallCenter::run()
{
    std::string query;

    std::cout<<"Enter a http request: ";
    std::cin>>query;

    while (query != "exit")
    {
        if(readRequest(query))
        {
            
        }else
        {

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