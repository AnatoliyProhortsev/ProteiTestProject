#include "../inc/Config.hpp"

Config::Config()
{
    performDefaultCfg();
}

void Config::performDefaultCfg()
{
    m_callDuplicationMode = false;
    m_operatorsCount = 3;
    m_processingTime = 10000;        //10000 mileseconds for 10 seconds
    m_queueSize = 10;
    m_Rmax = 10000;
    m_Rmin = 5000;
}

bool Config::readConfigFile(std::string fileName)
{
    std::fstream configFile(fileName);

    if(!configFile)
    {
        //error for reading cfg
        return false;
    }
    else
    {
        json parameters = json::parse(configFile);
        if(parameters.empty())
        {
            //error for parsing cfg
        }
        else
        {
            m_callDuplicationMode   = parameters["CallDuplicationMode"].template get<bool>();
            m_operatorsCount        = parameters["OperatorsCount"].template get<unsigned>();
            m_processingTime        = parameters["ProcessingTime"].template get<unsigned>();
            m_queueSize             = parameters["QueueSize"].template get<unsigned>();
            m_Rmax                  = parameters["Rmax"].template get<unsigned>();
            m_Rmin                  = parameters["Rmin"].template get<unsigned>();
        }
        return true;
    }
}

bool     Config::getCallDuplicationMode(){return m_callDuplicationMode;};
unsigned Config::getOperatorsCount(){return m_operatorsCount;};
unsigned Config::getProcessingTime(){return m_processingTime;};
unsigned Config::getQueueSize(){return m_queueSize;};
unsigned Config::getRmax(){return m_Rmax;};
unsigned Config::getRmin(){return m_Rmin;};