cmake_minimum_required(VERSION 3.24)

project(MTC VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(ENABLE_TDLIB "Enable TDLib integration when the library is available" OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(mtc
    src/main.cpp
    src/app/Application.cpp
    src/ui/MainWindow.cpp
    src/core/session/SessionManager.cpp
    src/core/telegram/TDLibAdapter.cpp
    src/core/calls/CallController.cpp
    src/core/devices/DeviceManager.cpp
    src/core/analytics/AnalyticsStore.cpp
)

target_include_directories(mtc PRIVATE src)

if(ENABLE_TDLIB)
    find_path(TDLIB_INCLUDE_DIR
        NAMES td/telegram/td_json_client.h
    )
    find_library(TDLIB_TDJSON_LIBRARY
        NAMES tdjson
    )

    if(TDLIB_INCLUDE_DIR AND TDLIB_TDJSON_LIBRARY)
        target_include_directories(mtc PRIVATE ${TDLIB_INCLUDE_DIR})
        target_link_libraries(mtc PRIVATE ${TDLIB_TDJSON_LIBRARY})
        target_compile_definitions(mtc PRIVATE MTC_HAS_TDLIB=1)
        message(STATUS "TDLib enabled")
    else()
        message(WARNING "ENABLE_TDLIB=ON, pero TDLib no fue encontrada. Se compilara la app sin integracion real.")
    endif()
endif()

target_link_libraries(mtc
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)

target_compile_options(mtc
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
