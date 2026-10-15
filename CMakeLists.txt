cmake_minimum_required(VERSION 3.20)

project(EndfieldNoLauncher VERSION 1.0.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "EndfieldNoLauncher targets Windows only. Cross-compile with cmake/mingw-w64-x86_64.cmake.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ON  (default): one self-contained executable, needs no redistributable.
# OFF: a smaller executable, but the target machine must provide the runtime
#      DLLs (msvcp140/vcruntime140 for MSVC, libstdc++-6/libgcc_s_seh-1/
#      libwinpthread-1 for mingw). Read by the target property at creation
#      time, so this must precede every target.
option(NOL_STATIC_RUNTIME
    "Link the C++ runtime into the executable instead of importing DLLs" ON)
if(NOL_STATIC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL>")
endif()

# The launcher core: discovery, signature verification, and process creation.
add_library(NoLauncherCore STATIC src/launch.cpp)
target_include_directories(NoLauncherCore PUBLIC src)

add_executable(EndfieldNoLauncher src/main.cpp)
target_link_libraries(EndfieldNoLauncher PRIVATE NoLauncherCore)

add_executable(EndfieldNoLauncherTests EXCLUDE_FROM_ALL tests/options_tests.cpp)
target_link_libraries(EndfieldNoLauncherTests PRIVATE NoLauncherCore)

set(NOL_TARGETS NoLauncherCore EndfieldNoLauncher EndfieldNoLauncherTests)

# _WIN32_WINNT 0x0601 selects the Vista process-attribute API:
# InitializeProcThreadAttributeList and EXTENDED_STARTUPINFO_PRESENT.
foreach(target IN LISTS NOL_TARGETS)
    target_compile_definitions(${target} PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601)
endforeach()

# wintrust: WinVerifyTrust and the signer chain helpers.
# crypt32:  CertGetNameStringW for the signer organization.
# shell32:  CommandLineToArgvW for the registered protocol command line.
target_link_libraries(NoLauncherCore PUBLIC wintrust crypt32 shell32)

if(MSVC)
    foreach(target IN LISTS NOL_TARGETS)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /EHsc)
        set_property(TARGET ${target} PROPERTY
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endforeach()
else()
    foreach(target IN LISTS NOL_TARGETS)
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endforeach()
    # -municode selects the wmain CRT entry point; mingw defaults to WinMain,
    # while MSVC needs no equivalent flag.
    # --gc-sections and -s drop the statically linked code that is unused.
    foreach(target EndfieldNoLauncher EndfieldNoLauncherTests)
        target_compile_options(${target} PRIVATE
            -municode -ffunction-sections -fdata-sections)
        target_link_options(${target} PRIVATE
            -municode -Wl,--gc-sections $<$<CONFIG:Release>:-s>
            $<$<BOOL:${NOL_STATIC_RUNTIME}>:-static>)
    endforeach()
endif()
