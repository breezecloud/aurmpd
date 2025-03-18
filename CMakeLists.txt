cmake_minimum_required(VERSION 3.1)

project (aurmpd)
# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")

option(WITH_MPD_HOST_CHANGE "Let users of the web frontend change the MPD Host" ON)
option(WITH_DYNAMIC_ASSETS "Serve assets dynamically (e.g for development/packaging)" OFF)
option(WITH_IPV6 "enable IPv6 support" ON)
option(WITH_SSL "enable SSL support" OFF)



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
endif()

configure_file(src/config.h.in ${PROJECT_BINARY_DIR}/config.h)
include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}	${LIBMPDCLIENT_INCLUDE_DIR})

include(CheckCSourceCompiles)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -pedantic")
if(WITH_IPV6)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS NS_ENABLE_IPV6)
endif()
if(WITH_SSL)
	find_package(OpenSSL REQUIRED)
	include_directories(${OPENSSL_INCLUDE_DIR})
#    list(APPEND LIB_LIST ${OPENSSL_LIBRARIES})
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS NS_ENABLE_SSL)
endif()

set(SOURCES
    src/main.cpp
    src/mpd_client.c
    src/mongoose.c
    src/json_encode.c
    src/http_server.cpp
    src/song_queue.cpp
)

if(WIN32)
    add_executable(winaurmpd src/winmain.c)
    target_link_libraries(winaurmpd PRIVATE ws2_32  ${OLE32_LIBRARY} ${UUID_LIBRARY}) 

    add_executable(aurmpd ${SOURCES})
    target_link_libraries(aurmpd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES} ws2_32) 
elseif(UNIX)
    add_executable(aurmpd ${SOURCES})
    target_link_libraries(aurmpd ${LIBMPDCLIENT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES}) 
endif()

# install(TARGETS ympd DESTINATION bin)
# install(FILES ympd.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
# if(WITH_DYNAMIC_ASSETS)
#     install(DIRECTORY htdocs DESTINATION share/${PROJECT_NAME})
# endif()
