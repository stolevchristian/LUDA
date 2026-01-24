cmake_minimum_required(VERSION 4.1)
project(LUDA)

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)  # or your Windows SDK version
endif()

# Because obviously??
set(VCPKG_TARGET_TRIPLET "x64-windows-static")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Recursively find all .cpp files in ida_sdk
file(GLOB_RECURSE IDA_SDK_SOURCES
        "${CMAKE_SOURCE_DIR}/IdaSDK/*.cpp"
)
file(GLOB_RECURSE LUDASOCKET_SOURCES
        "${CMAKE_SOURCE_DIR}/LudaSocket/*.cpp"
)
file(GLOB_RECURSE EXECUTOR_SOURCES
        "${CMAKE_SOURCE_DIR}/Executor/*.cpp"
)
file(GLOB_RECURSE LUA_SOURCES
        "${CMAKE_SOURCE_DIR}/Lua/*.cpp"
        "${CMAKE_SOURCE_DIR}/Lua/*.c"
)
add_library(LUDA SHARED
        Plugin.cpp
        ${IDA_SDK_SOURCES}
        ${LUDASOCKET_SOURCES}
        ${EXECUTOR_SOURCES}
        ${LUA_SOURCES}
)

file(GLOB IDA_LIBS "${CMAKE_SOURCE_DIR}/IdaSDK/libs/*.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${IDA_LIBS}
        ${CMAKE_SOURCE_DIR}/keystone/keystone.lib
)

# Makes everything just look better, maybe just my opinion
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/IdaSDK
)
