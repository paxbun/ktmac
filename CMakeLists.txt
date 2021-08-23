cmake_minimum_required(VERSION 3.13)
project(kakao)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# ------------------------------------------- Options  ------------------------------------------- #

option(KTMAC_BUILD_MAIN "Specifies whether the main executable is to be built." ON)
option(KTMAC_SHARED_CORE "Specifies whether the core library is to be built as a shared library." OFF)
option(KTMAC_BUILD_TESTS "Specifies whether the test executables are to be built." OFF)

# ----------------------------- Essential libraries and executables  ----------------------------- #

if (KTMAC_BUILD_MAIN)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
endif()

add_library(ktmac-process-watcher-socket ${PROJECT_SOURCE_DIR}/Source/ProcessWatcherSocket.cc)
target_link_libraries(ktmac-process-watcher-socket Ws2_32.lib)
target_include_directories(ktmac-process-watcher-socket PUBLIC ${PROJECT_SOURCE_DIR}/Public)

add_library(ktmac-window-hook SHARED ${PROJECT_SOURCE_DIR}/Source/WindowHook.cc)
target_include_directories(ktmac-window-hook PUBLIC ${PROJECT_SOURCE_DIR}/Public)
target_compile_definitions(ktmac-window-hook PRIVATE -DHOOK_EXPORT)

add_executable(ktmac-process-hook WIN32
    ${PROJECT_SOURCE_DIR}/Source/ProcessHook.cc
    ${PROJECT_SOURCE_DIR}/Source/ProcessWatcher.cc
    ${PROJECT_SOURCE_DIR}/Source/WmiEventSink.cc
)
target_link_libraries(ktmac-process-hook ktmac-process-watcher-socket wbemuuid.lib)
target_include_directories(ktmac-process-hook PUBLIC ${PROJECT_SOURCE_DIR}/Public)

set(KTMAC_CORE_TYPE STATIC)
if (KTMAC_BUILD_MAIN OR KTMAC_SHARED_CORE)
    set(KTMAC_CORE_TYPE SHARED)
endif()

add_library(ktmac-core ${KTMAC_CORE_TYPE} ${PROJECT_SOURCE_DIR}/Source/KakaoStateManager.cc)
target_link_libraries(ktmac-core PUBLIC ktmac-process-watcher-socket ktmac-window-hook)
target_include_directories(ktmac-core PUBLIC ${PROJECT_SOURCE_DIR}/Public)
add_dependencies(ktmac-core ktmac-process-hook)

if (KTMAC_CORE_TYPE MATCHES "STATIC")
    target_compile_definitions(ktmac-core PUBLIC -DKTMAC_CORE_STATIC)
else()
    target_compile_definitions(ktmac-core
        PUBLIC -DKTMAC_CORE_SHARED
        PRIVATE -DKTMAC_CORE_EXPORT
        INTERFACE -DKTMAC_CORE_IMPORT
    )
endif()

# -------------------------------------------- Tests  -------------------------------------------- #

if (KTMAC_BUILD_TESTS)
    add_executable(ktmac-test ${PROJECT_SOURCE_DIR}/Tests/KtmacTest.cc)
    target_link_libraries(ktmac-test ktmac-core)

    add_executable(ktmac-process-hook-test ${PROJECT_SOURCE_DIR}/Tests/KtmacProcessHookTest.cc)
    target_link_libraries(ktmac-process-hook-test ktmac-process-watcher-socket)
endif()

# --------------------------------------- Main executable  --------------------------------------- #

if (KTMAC_BUILD_MAIN)
    find_package(CEF REQUIRED)
    add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
    add_logical_target("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
    set_cef_target_out_dir()

    add_executable(ktmac WIN32
        ${PROJECT_SOURCE_DIR}/Resources/ktmac/gui/Resources.rc
        ${PROJECT_SOURCE_DIR}/Source/gui/App.cc
        ${PROJECT_SOURCE_DIR}/Source/gui/Handler.cc
        ${PROJECT_SOURCE_DIR}/Source/gui/Main.cc
        ${PROJECT_SOURCE_DIR}/Source/gui/ResourceUtils.cc
    )
    set_source_files_properties(${PROJECT_SOURCE_DIR}/Resources/ktmac/gui/Resources.rc LANGUAGE RC)
    add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/Resources/ktmac/gui/Resources.rc
        COMMAND ${CMAKE_COMMAND} -E touch ${PROJECT_SOURCE_DIR}/Resources/ktmac/gui/Resources.rc
        DEPENDS ${PROJECT_SOURCE_DIR}/Resources/ktmac/gui/View.html
        COMMENT "Resource files"
    )

    set_executable_target_properties(ktmac)
    target_link_libraries(ktmac ktmac-core libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})
    target_include_directories(ktmac PRIVATE ${PROJECT_SOURCE_DIR}/Resources)

    add_logical_target("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    target_link_libraries(ktmac cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
    
    add_windows_manifest("${PROJECT_SOURCE_DIR}/Resources/ktmac/gui" "ktmac" "exe")
    copy_files(ktmac "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
    copy_files(ktmac "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")
endif()