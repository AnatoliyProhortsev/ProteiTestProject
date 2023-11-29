cmake_minimum_required(VERSION 3.10)

# set the project name
project(CallCenter)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/lib)

# add the executable
file(GLOB SOURCES "src/Config.cpp", "src/CallCenter.cpp", "src/test.cpp")

# add the test executable
file(GLOB TEST_SOURCES "src/Config.cpp", "src/CallCenter.cpp", "src/performTests.cpp")

#Add the libraries
include(FetchContent)
FetchContent_Declare(
  spdlog
  URL https://github.com/gabime/spdlog/archive/refs/heads/v1.x.zip
)
FetchContent_Declare(
  httplib
  URL https://github.com/yhirose/cpp-httplib/archive/refs/heads/master.zip
)
FetchContent_MakeAvailable(spdlog httplib)

#Add the main executable
add_executable(CallCenter ${SOURCES})
#Add compiler options for the main executable
target_compile_options(CallCenter PUBLIC -pthread)
#Add the libraries to the main executable
target_link_libraries(CallCenter httplib spdlog)

# Download and install GoogleTest
include(ExternalProject)
ExternalProject_Add(
  GTest
  URL https://github.com/google/googletest/archive/release-1.8.0.zip
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
  BUILD_COMMAND sudo make all
  INSTALL_COMMAND sudo make install
)
ExternalProject_Get_Property(GTest source_dir binary_dir)

find_package(GTest REQUIRED)
#Add the test executable
add_executable(CallCenterTest ${TEST_SOURCES})
#Add compiler options for the test executable
target_compile_options(CallCenterTest PUBLIC -pthread -lgtest)
#Add the libraries to the test executable
add_dependencies(CallCenterTest GTest)
target_link_libraries(CallCenterTest httplib spdlog GTest::GTest)

##################################
# Just make the test runnable with
#   $ make test 
enable_testing()
add_test(NAME    CallCenterTest 
         COMMAND CallCenterTest)




