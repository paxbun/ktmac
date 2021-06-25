cmake_minimum_required(VERSION 3.13)
project(kakao)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ktmac-window-hook SHARED
    ${PROJECT_SOURCE_DIR}/Source/WindowHook.cc
)
target_include_directories(ktmac-window-hook PUBLIC
    ${PROJECT_SOURCE_DIR}/Public
)
target_compile_definitions(ktmac-window-hook PRIVATE
    -DHOOK_EXPORT
)

add_library(ktmac
    ${PROJECT_SOURCE_DIR}/Source/KakaoStateManager.cc
)
target_link_libraries(ktmac PUBLIC ktmac-window-hook)
target_include_directories(ktmac PUBLIC
    ${PROJECT_SOURCE_DIR}/Public
)

add_executable(ktmac-test
    ${PROJECT_SOURCE_DIR}/Source/Main.cc
)
target_link_libraries(ktmac-test PUBLIC ktmac)
