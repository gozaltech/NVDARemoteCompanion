cmake_minimum_required(VERSION 3.22)
project(nvdaremote)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

set(SHARED_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src")

include(FetchContent)

set(JSON_DisableEnumSerialization ON)
set(JSON_ImplicitConversions OFF)
set(JSON_Diagnostics OFF)
set(JSON_Install OFF)
set(JSON_MultipleHeaders OFF)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
set(INSTALL_MBEDTLS_HEADERS OFF CACHE BOOL "" FORCE)
set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
set(ENABLE_DOC OFF CACHE BOOL "" FORCE)
set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_Declare(mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG v3.6.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(mbedtls)

add_library(nvdaremote SHARED
    AndroidBridge.cpp
    AndroidSpeech.cpp
    AndroidAudio.cpp

    ${SHARED_SRC}/NetworkClient.cpp
    ${SHARED_SRC}/SSLClient.cpp
    ${SHARED_SRC}/ConnectionManager.cpp
    ${SHARED_SRC}/ConfigFile.cpp
    ${SHARED_SRC}/KeyboardState.cpp
    ${SHARED_SRC}/MessageSender.cpp
    ${SHARED_SRC}/AppState.cpp
    ${SHARED_SRC}/Debug.cpp
)

target_include_directories(nvdaremote PRIVATE
    ${SHARED_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(nvdaremote PRIVATE
    android
    log
    mbedtls mbedcrypto mbedx509
    nlohmann_json::nlohmann_json
)

target_compile_definitions(nvdaremote PRIVATE
    ANDROID
    NLOHMANN_JSON_DISABLE_ENUM_SERIALIZATION
    JSON_USE_IMPLICIT_CONVERSIONS=0
    JSON_DIAGNOSTICS=0
    NDEBUG
)
