#include "../inc/Config.hpp"

Config::Config()
{
    performDefaultCfg();
    m_currentCfgFileName = "";
}

Config::Config(const std::string &fileName)
{
    m_currentCfgFileName = "";
    if(!readConfigFile(fileName))
    {
        spdlog::debug("Can't read cfg file. Default cfg params will proceed.");
        performDefaultCfg();
    }
    spdlog::debug("New cfg file applied.");
}

void Config::performDefaultCfg()
{
    m_operatorsCount = 3;
    m_processingTime = 10000;        //10000 mileseconds for 10 seconds
    m_queueSize = 10;
    m_Rmax = 10000;
    m_Rmin = 5000;
}

bool Config::readConfigFile(const std::string &fileName)
{
    std::fstream configFile(fileName);

    if(!configFile)
    {
        if(m_currentCfgFileName != "")
            return readConfigFile(m_currentCfgFileName);
        
        return false;
    }
    else
    {
        json parameters = json::parse(configFile);
        if(parameters.empty() || parameters.size() != 5)
        {
            if(m_currentCfgFileName != "")
                    return readConfigFile(m_currentCfgFileName);

                return false;
        }
        else
        {
            m_operatorsCount      = 
                parameters["OperatorsCount"].template get<unsigned>();
            m_processingTime      = 
                parameters["ProcessingTime"].template get<unsigned>();
            m_queueSize           = 
                parameters["QueueSize"].template get<unsigned>();
            m_Rmax                = 
                parameters["Rmax"].template get<unsigned>();
            m_Rmin                = 
                parameters["Rmin"].template get<unsigned>();

            if(m_operatorsCount < 1 ||
                m_processingTime < 500 ||
                m_queueSize < 1 ||
                m_Rmax < 500 ||
                m_Rmin < 500 ||
                m_Rmax < m_Rmin)
            {
                spdlog::debug("One or more invalid cfg params.");
                if(m_currentCfgFileName != "")
                    return readConfigFile(m_currentCfgFileName);

                return false;
            }
        }
        m_currentCfgFileName = fileName;
        configFile.close();
        return true;
    }
}

unsigned Config::getOperatorsCount(){return m_operatorsCount;};
unsigned Config::getProcessingTime(){return m_processingTime;};
unsigned Config::getQueueSize(){return m_queueSize;};
unsigned Config::getRmax(){return m_Rmax;};
unsigned Config::getRmin(){return m_Rmin;};