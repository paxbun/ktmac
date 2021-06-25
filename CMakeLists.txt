cmake_minimum_required(VERSION 3.13)
project(kakao)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ktmac-process-watcher-socket
    ${PROJECT_SOURCE_DIR}/Source/ProcessWatcherSocket.cc
)
target_link_libraries(ktmac-process-watcher-socket Ws2_32.lib)
target_include_directories(ktmac-process-watcher-socket PUBLIC
    ${PROJECT_SOURCE_DIR}/Public
)

add_library(ktmac-window-hook SHARED
    ${PROJECT_SOURCE_DIR}/Source/WindowHook.cc
)
target_include_directories(ktmac-window-hook PUBLIC
    ${PROJECT_SOURCE_DIR}/Public
)
target_compile_definitions(ktmac-window-hook PRIVATE
    -DHOOK_EXPORT
)

add_executable(ktmac-process-hook WIN32
    ${PROJECT_SOURCE_DIR}/Source/ProcessHook.cc
    ${PROJECT_SOURCE_DIR}/Source/ProcessWatcher.cc
    ${PROJECT_SOURCE_DIR}/Source/WmiEventSink.cc
)
target_link_libraries(ktmac-process-hook ktmac-process-watcher-socket wbemuuid.lib)
target_include_directories(ktmac-process-hook PUBLIC
    ${PROJECT_SOURCE_DIR}/Public
)

add_library(ktmac
    ${PROJECT_SOURCE_DIR}/Source/KakaoStateManager.cc
)
target_link_libraries(ktmac PUBLIC ktmac-process-watcher-socket ktmac-window-hook)
target_include_directories(ktmac PUBLIC
    ${PROJECT_SOURCE_DIR}/Public
)

add_executable(ktmac-test
    ${PROJECT_SOURCE_DIR}/Source/Main.cc
)
target_link_libraries(ktmac-test PUBLIC ktmac)

add_executable(ktmac-process-hook-test
    ${PROJECT_SOURCE_DIR}/Source/ProcessHookTest.cc
)
target_link_libraries(ktmac-process-hook-test ktmac-process-watcher-socket)
target_include_directories(ktmac PUBLIC
    ${PROJECT_SOURCE_DIR}/Public
)