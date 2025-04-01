cmake_minimum_required(VERSION 3.1)

project (aurmpd)
# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "0")

option(WITH_IPV6 "enable IPv6 support" ON)

find_package(LibMPDClient REQUIRED)
find_package(Threads REQUIRED)

if(WIN32)
    # 手动指定 Windows SDK 路径
    set(WINSDK_DIR "C:/Program Files (x86)/Windows Kits/10" CACHE PATH "Path to Windows SDK")
    set(TARGET_PLATFORM "x64")
    set(WINSDK_LIB_DIR "${WINSDK_DIR}/Lib/10.0.20348.0/um/${TARGET_PLATFORM}" CACHE PATH "Path to Windows SDK libraries")

    # 将 SDK 库路径添加到搜索路径
    list(APPEND CMAKE_LIBRARY_PATH ${WINSDK_LIB_DIR})
    # 查找 ole32 库
    find_library(OLE32_LIBRARY Ole32)
    if(NOT OLE32_LIBRARY)
        message(FATAL_ERROR "ole32 library not found")
    endif()
    # 查找 uuid 库
    find_library(UUID_LIBRARY uuid)
    if(NOT UUID_LIBRARY)
        message(FATAL_ERROR "uuid library not found")
    endif()
    # 设置资源文件路径
    set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/src/resource.rc")
    # 设置资源编译器为 windres（MinGW）
    set(CMAKE_RC_COMPILER windres)
    # 定义资源文件的编译命令（生成COFF格式对象文件）
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>"
    )
    enable_language(RC)  # 启用对资源文件的支持
endif()

configure_file(src/config.h.in ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}	${LIBMPDCLIENT_INCLUDE_DIR})

include(CheckCSourceCompiles)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -pedantic")

set(SOURCES
    src/main.cpp
    src/mpd_client.c
    src/mongoose.c
    src/json_encode.c
    src/http_server.cpp
    src/song.cpp
    src/queue.cpp
    src/library.cpp
    src/album.cpp
    src/mpdqueue.cpp
)

if(WIN32)
    add_executable(winaurmpd src/winmain.c ${RESOURCE_FILE})
    target_link_libraries(winaurmpd PRIVATE ws2_32  ${OLE32_LIBRARY} ${UUID_LIBRARY}) 

    add_executable(aurmpd ${SOURCES} src/winpipe.cpp)
    target_link_libraries(aurmpd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES} ws2_32) 
elseif(UNIX)
    add_executable(aurmpd ${SOURCES})
    target_link_libraries(aurmpd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES}) 
endif()
