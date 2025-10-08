cmake_minimum_required (VERSION 3.12)

set(proj_name "minecraftspeedproxy")
project (${proj_name})
set(CMAKE_CXX_STANDARD 20)

EXECUTE_PROCESS(COMMAND git describe --tags --always --dirty=-dev
        TIMEOUT 5
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
MESSAGE(STATUS "building version ${GIT_VERSION}")
add_definitions(-DBUILD_VERSION=\"${GIT_VERSION}\")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	add_definitions(-DDEBUG)
else ()
	add_definitions(-DRELEASE)
endif ()

file(GLOB RBS_LIB_HEADERS "rbslib/*.h" "rbslib/Windows/*.h")

file(GLOB RBS_LIB_SRC "rbslib/*.cpp" "rbslib/Windows/*.cpp")

file(GLOB JSON_LIB_HEADERS "json/*.h")
file(GLOB JSON_LIB_SRC "json/*.cpp")

include_directories("asio/include")

# 将源代码添加到此项目的可执行文件。
# 若需要二次开发请修改下面指令中的文件名
add_executable (minecraftspeedproxy 
"asio/import_asio.h" "asio/import_asio.cpp"
"main.cpp" 
"WhiteBlackList.cpp" "WhiteBlackList.h"
"config.cpp" "config.h"
"datapackage.cpp" "datapackage.h"
"logger.cpp" "logger.h"
"proxy.cpp" "proxy.h"
"helper.cpp" "helper.h"
"WebControl.cpp" "WebControl.h"
${RBS_LIB_SRC} ${JSON_LIB_SRC} )

# Windows平台链接ws2_32.lib
if (WIN32)
  target_link_libraries(minecraftspeedproxy ws2_32.lib)
endif()

set_property(TARGET minecraftspeedproxy PROPERTY CXX_STANDARD 20)

