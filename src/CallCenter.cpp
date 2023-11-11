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
    std::time_t date = time(0);
    //Выгружать CDR с названием файла текущая_дата.txt
}