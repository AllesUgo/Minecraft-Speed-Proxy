# CMakeList.txt: rbslib 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(proj_name "minecraftspeedproxy")
project (${proj_name})
if(MSVC)
    # Windows上的编译器设置 (MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    message("Use GNU compiler")
    #静态链接glibc
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libstdc++ -static-libgcc")
endif()
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
	# -UDEV_CODE 表示取消 DEV_CODE 的定义
	add_definitions(-DRELEASE)
endif ()

file(GLOB RBS_LIB_HEADERS "rbslib/*.h" "rbslib/Windows/*.h")

file(GLOB RBS_LIB_SRC "rbslib/*.cpp" "rbslib/Windows/*.cpp")

file(GLOB JSON_LIB_HEADERS "json/*.h")
file(GLOB JSON_LIB_SRC "json/*.cpp")

# 将源代码添加到此项目的可执行文件。
#若需要二次开发请修改下面指令中的文件名
add_executable (minecraftspeedproxy "main.cpp" "WhiteBlackList.cpp" "config.cpp" "datapackage.cpp" "logger.cpp" "proxy.cpp" "helper.cpp" ${RBS_LIB_SRC} ${JSON_LIB_SRC} )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET minecraftspeedproxy PROPERTY CXX_STANDARD 20)
endif()

# TODO: 如有需要，请添加测试并安装目标。
