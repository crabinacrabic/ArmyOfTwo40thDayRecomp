# armyoftwo40threcomp - Army of Two: The 40th Day PC Recompilation
# Based on ReXGlue SDK v0.10.0+
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(armyoftwo40threcomp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REXSDK_VERSION "0.10.0.8-dev.g1406e1b")

if(WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64)$")
    set(REXSDK_PLATFORM "win-amd64")
elseif(WIN32 AND CMAKE_SYSTEM_PROCESSOR MATCHES "^ARM64$")
    message(FATAL_ERROR "win-arm64 ReXGlue SDK release is not published")
elseif(APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
    set(REXSDK_PLATFORM "mac-arm64")
elseif(APPLE)
    set(REXSDK_PLATFORM "mac-amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
    set(REXSDK_PLATFORM "linux-arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
    set(REXSDK_PLATFORM "linux-amd64")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND}
            "-DREXSDK_VERSION='${REXSDK_VERSION}'"
            "-DREXSDK_PLATFORM=${REXSDK_PLATFORM}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fetch-rexglue-sdk.cmake")

set(REXSDK_VERSION "")

# Find the fetched SDK with any preset, not only the local-* ones from
# CMakeUserPresets.json (Visual Studio configures the first preset on open).
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/rexglue/${REXSDK_PLATFORM}")

# First configure on a fresh clone: generated/default/sources.cmake only exists
# after `rexglue codegen`, and without it the recomp target is not created.
# assets/default.xex must be in place before this runs.
set(ARMYOFTWO40THRECOMP_GENERATED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/generated/default/sources.cmake")
if(NOT EXISTS "${ARMYOFTWO40THRECOMP_GENERATED_SOURCES}")
    set(ARMYOFTWO40THRECOMP_REXGLUE_EXE
        "${CMAKE_CURRENT_SOURCE_DIR}/rexglue/${REXSDK_PLATFORM}/bin/rexglue${CMAKE_EXECUTABLE_SUFFIX}")
    if(EXISTS "${ARMYOFTWO40THRECOMP_REXGLUE_EXE}")
        message(STATUS "[armyoftwo40threcomp] generated/default/sources.cmake missing - running codegen once")
        execute_process(
            COMMAND "${ARMYOFTWO40THRECOMP_REXGLUE_EXE}" codegen "${CMAKE_CURRENT_SOURCE_DIR}/armyoftwo40threcomp_manifest.toml"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE ARMYOFTWO40THRECOMP_CODEGEN_RESULT)
        if(NOT ARMYOFTWO40THRECOMP_CODEGEN_RESULT EQUAL 0)
            message(WARNING "[armyoftwo40threcomp] bootstrap codegen failed (${ARMYOFTWO40THRECOMP_CODEGEN_RESULT}); "
                            "the build-time codegen step will report the errors")
        endif()
    endif()
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ARMYOFTWO40THRECOMP_GENERATED_SOURCES}")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/rexglue.cmake")
    include(generated/rexglue.cmake)
endif()

set(ARMYOFTWO40THRECOMP_SOURCES
    src/main.cpp
)

if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/armyoftworecomp.rc")
        list(APPEND ARMYOFTWO40THRECOMP_SOURCES
            src/armyoftworecomp.rc
        )
    endif()
endif()

if(WIN32)
    add_executable(armyoftwo40threcomp WIN32 ${ARMYOFTWO40THRECOMP_SOURCES})
else()
    add_executable(armyoftwo40threcomp ${ARMYOFTWO40THRECOMP_SOURCES})
endif()

if(COMMAND rexglue_setup_target)
    rexglue_setup_target(armyoftwo40threcomp GPU_PLUGINS xenos)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/settings/gamecontrollerdb.txt")
    add_custom_command(TARGET armyoftwo40threcomp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/settings/gamecontrollerdb.txt"
                "$<TARGET_FILE_DIR:armyoftwo40threcomp>/gamecontrollerdb.txt"
        COMMENT "Copying gamecontrollerdb.txt next to armyoftwo40threcomp.exe")
endif()
