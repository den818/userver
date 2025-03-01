set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

option(USERVER_DOWNLOAD_PACKAGE_AMQPCPP "Download and setup amqp-cpp" ${USERVER_DOWNLOAD_PACKAGES})
if (NOT USERVER_DOWNLOAD_PACKAGE_AMQPCPP)
  MESSAGE(FATAL_ERROR "Please enable USERVER_DOWNLOAD_PACKAGE_AMQPCPP, otherwise rabbitmq driver can't be built")
endif()

include(FetchContent)
FetchContent_Declare(
  amqp-cpp_external_project
  GIT_REPOSITORY https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
  TIMEOUT 10
  GIT_TAG v4.3.16
  SOURCE_DIR ${USERVER_ROOT_DIR}/third_party/amqp-cpp
)
FetchContent_GetProperties(amqp-cpp_external_project)
if(NOT amqp-cpp_external_project_POPULATED)
    message(STATUS "Downloading amqp-cpp from remote")
    FetchContent_Populate(amqp-cpp_external_project)
endif()

add_subdirectory(${USERVER_ROOT_DIR}/third_party/amqp-cpp "${CMAKE_BINARY_DIR}/third_party/amqp-cpp")
add_library(amqp-cpp ALIAS amqpcpp)
set(amqp-cpp_VERSION "4.3.16" CACHE STRING "Version of the amqp-cpp")
