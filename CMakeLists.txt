cmake_minimum_required(VERSION 3.28)
project(NVDARemoteCompanion VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NVDA_VERSION "2025.2" CACHE STRING "NVDA version to download")

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL /Gy /GF /Gw /favor:INTEL64")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF /OPT:LBR /MERGE:.rdata=.text")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -flto=auto -fdata-sections -ffunction-sections -march=native")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto=auto -Wl,--gc-sections -Wl,--strip-all -Wl,--strip-debug -Wl,-z,norelro")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(JSON_DisableEnumSerialization ON)
set(JSON_ImplicitConversions OFF)
set(JSON_Diagnostics OFF)
set(JSON_Install OFF)
set(JSON_MultipleHeaders OFF)

FetchContent_Declare(json 
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(json)

FetchContent_Declare(mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG v3.6.4
    GIT_SHALLOW TRUE
)

set(SRAL_BUILD_SHARED OFF CACHE BOOL "Build SRAL as shared library" FORCE)
set(SRAL_BUILD_STATIC ON CACHE BOOL "Build SRAL as static library" FORCE)
set(BUILD_SRAL_TEST OFF CACHE BOOL "Build SRAL examples/tests" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" CACHE STRING "MSVC runtime library" FORCE)

FetchContent_Declare(sral
    GIT_REPOSITORY https://github.com/m1maker/sral.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

set(ENABLE_PROGRAMS OFF)
set(ENABLE_TESTING OFF)
set(MBEDTLS_FATAL_WARNINGS OFF)
set(INSTALL_MBEDTLS_HEADERS OFF)
set(USE_SHARED_MBEDTLS_LIBRARY OFF)
set(ENABLE_DOC OFF)
set(DISABLE_PACKAGE_CONFIG_AND_INSTALL ON)


FetchContent_MakeAvailable(mbedtls)

FetchContent_MakeAvailable(sral)

if(WIN32)
    FetchContent_Declare(nvda_controller_client
        URL https://download.nvaccess.org/releases/${NVDA_VERSION}/nvda_${NVDA_VERSION}_controllerClient.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nvda_controller_client)
endif()

set(COMMON_SOURCES
    src/main.cpp
    src/NetworkClient.cpp
    src/ConnectionManager.cpp
    src/SSLClient.cpp
    src/Debug.cpp
    src/Speech.cpp
    src/Audio.cpp
)

if(WIN32)
    list(APPEND COMMON_SOURCES 
        src/KeyboardState.cpp
        src/AppState.cpp
        src/EventChecker.cpp
        src/MessageSender.cpp
        src/KeyboardHook.cpp
    )
endif()

add_executable(nvda_remote_companion ${COMMON_SOURCES})

target_include_directories(nvda_remote_companion PRIVATE src)
target_include_directories(nvda_remote_companion PRIVATE ${sral_SOURCE_DIR}/include)
target_link_libraries(nvda_remote_companion PRIVATE mbedtls mbedcrypto mbedx509 nlohmann_json::nlohmann_json SRAL_static)

target_compile_definitions(nvda_remote_companion PRIVATE
    NLOHMANN_JSON_DISABLE_ENUM_SERIALIZATION
    JSON_DISABLE_ENUM_SERIALIZATION
    JSON_USE_IMPLICIT_CONVERSIONS=0
    JSON_DIAGNOSTICS=0
    NDEBUG
    SRAL_STATIC
    _FORTIFY_SOURCE=0
)

if(WIN32)
    target_link_libraries(nvda_remote_companion PRIVATE user32 uiautomationcore winmm)
    target_compile_definitions(nvda_remote_companion PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0601
    )
else()
    find_library(SPEECHD_LIB speechd)
    find_library(BRLAPI_LIB brlapi)
    
    if(SPEECHD_LIB)
        target_link_libraries(nvda_remote_companion PRIVATE ${SPEECHD_LIB})
    endif()
    
    if(BRLAPI_LIB)
        target_link_libraries(nvda_remote_companion PRIVATE ${BRLAPI_LIB})
    else()
        message(WARNING "BrlAPI library not found. Install with: sudo apt install libbrlapi-dev")
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_link_options(nvda_remote_companion PRIVATE /DEBUG:NONE /INCREMENTAL:NO)
        set_target_properties(nvda_remote_companion PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
    else()
        target_link_options(nvda_remote_companion PRIVATE -s)
    endif()
endif()

set_target_properties(nvda_remote_companion PROPERTIES
    WIN32_EXECUTABLE FALSE
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(NVDA_ARCH_DIR "ARM64")
        else()
            set(NVDA_ARCH_DIR "x64")
        endif()
    else()
        set(NVDA_ARCH_DIR "x86")
    endif()
    
    if(CMAKE_GENERATOR_PLATFORM)
        if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
            set(NVDA_ARCH_DIR "ARM64")
        elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
            set(NVDA_ARCH_DIR "x64")
        elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
            set(NVDA_ARCH_DIR "x86")
        endif()
    endif()
    
    message(STATUS "Using NVDA Controller Client from: ${NVDA_ARCH_DIR}")
    
    FetchContent_GetProperties(nvda_controller_client)
    if(NOT nvda_controller_client_POPULATED)
        FetchContent_Populate(nvda_controller_client)
    endif()
    
    add_custom_command(TARGET nvda_remote_companion POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${nvda_controller_client_SOURCE_DIR}/${NVDA_ARCH_DIR}/nvdaControllerClient.dll
        $<TARGET_FILE_DIR:nvda_remote_companion>/nvdaControllerClient.dll
        COMMENT "Copying nvdaControllerClient.dll from ${NVDA_ARCH_DIR} to binary directory"
    )
endif()