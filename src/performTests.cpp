#include <stdio.h>
#include <stdlib.h>

#include "../inc/CallCenter.hpp"

CallCenter cc("../cfg/defaultCdf.json");

TEST(test_call, just_call)
{
    system("curl -d \"number=89315532080\" -X POST http://localhost:8080/call");
    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));

    ASSERT_TRUE(cc.m_CDRvec[0].m_callerNumber == "89315532080");
    ASSERT_TRUE(cc.m_CDRvec[0].m_callStatus == "OK");
}

TEST(test_call, call_duplication)
{
    system("curl -d \"number=89315532080\" -X POST http://localhost:8080/call");
    system("curl -d \"number=89315532080\" -X POST http://localhost:8080/call");

    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));

    ASSERT_TRUE(cc.m_CDRvec.size() == 2);
    ASSERT_TRUE(cc.m_CDRvec[1].m_callerNumber == "89315532080");
    ASSERT_TRUE(cc.m_CDRvec[1].m_callStatus == "OK");
}

TEST(test_call, wrong_cmd)
{
    system("curl -d \"numer=89315532080\" -X POST http://localhost:8080/call");
    system("curl -d \"number=89315532080\" -X POST http://localhost:8080/call");

    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));

    ASSERT_TRUE(cc.m_CDRvec.size() == 3);
    ASSERT_TRUE(cc.m_CDRvec[2].m_callerNumber == "89315532080");
    ASSERT_TRUE(cc.m_CDRvec[2].m_callStatus == "OK");
}

TEST(test_call, wrong_number)
{
    system("curl -d \"number=8931553208\" -X POST http://localhost:8080/call");
    system("curl -d \"number=8931532080f\" -X POST http://localhost:8080/call");
    system("curl -d \"number=\" -X POST http://localhost:8080/call");

    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));

    ASSERT_TRUE(cc.m_CDRvec.size() == 3);
}

TEST(config, wrong_init_cfg)
{
    system("curl -d \"number=89315532080\" -X POST http://localhost:8080/call");
    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));
    
    ASSERT_TRUE(cc.m_config.getCfgFileName() == "");
    ASSERT_TRUE(cc.m_CDRvec.back().m_callerNumber == "89315532080");
    ASSERT_TRUE(cc.m_CDRvec.back().m_callStatus == "OK");
}

//Можно добавить тест про ограничение очереди и кол-ва операторов

TEST(config, valid_init_cfg)
{
    system("curl -d \"name=../cfg/defaultCfg.json\" -X POST http://localhost:8080/config");
    system("curl -d \"number=89315532080\" -X POST http://localhost:8080/call");

    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));

    ASSERT_TRUE(cc.m_config.getCfgFileName() == "../cfg/defaultCfg.json");
    ASSERT_TRUE(cc.m_CDRvec.back().m_callerNumber == "89315532080");
    ASSERT_TRUE(cc.m_CDRvec.back().m_callStatus == "OK");
}

TEST(config, valid_cfg_in_run)
{
    ASSERT_TRUE(cc.m_config.getCfgFileName() == "../cfg/defaultCfg.json");

    system("curl -d \"number=89315532080\" -X POST http://localhost:8080/call");
    system("curl -d \"name=../cfg/cfg.json\" -X POST http://localhost:8080/config");

    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));

    ASSERT_TRUE(cc.m_config.getCfgFileName() == "../cfg/cfg.json");
    ASSERT_TRUE(cc.m_CDRvec.back().m_callerNumber == "89315532080");
    ASSERT_TRUE(cc.m_CDRvec.back().m_callStatus == "OK");

    system("curl -d \"number=89315533080\" -X POST http://localhost:8080/call");

    std::this_thread::sleep_for(
        std::chrono::milliseconds(
            cc.m_config.getProcessingTime() + cc.m_config.getRmax()));

    ASSERT_TRUE(cc.m_CDRvec.back().m_callerNumber == "89315533080");
    ASSERT_TRUE(cc.m_CDRvec.back().m_callStatus == "OK");

    system("curl -d \"\" -X POST http://localhost:8080/shutdown");
}

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::off);
    ::testing::InitGoogleTest(&argc, argv);

    std::thread tester(&CallCenter::start, &cc);
	return RUN_ALL_TESTS();;
}