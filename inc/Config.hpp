#include <fstream>
#include <string>

#include "../lib/json/json.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <gtest/gtest.h>

using json = nlohmann::json;

class Config
{
public:
                Config();
                Config(const std::string &fileName);
    bool        readConfigFile(const std::string &fileName);
    unsigned    getOperatorsCount();
    unsigned    getQueueSize();
    unsigned    getRmin();
    unsigned    getRmax();
    unsigned    getProcessingTime();
    void        performDefaultCfg();
    std::string getCfgFileName();
private:
    std::string m_currentCfgFileName;
    unsigned    m_operatorsCount;
    unsigned    m_queueSize;
    unsigned    m_Rmin;
    unsigned    m_Rmax;
    unsigned    m_processingTime;
};