cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(obs-studio VERSION ${OBS_VERSION_CANONICAL})

if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/architecture.cmake")
  if(NOT OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_VS_PLATFORM_NAME)
    return()
  endif()
endif()

# obs-browser defaults ENABLE_BROWSER to OFF and then never registers
# browser_source. Scenes saved in OBS show those sources with red names and a
# generic icon. On Windows, turn the module on unless the user opted out, so
# dependency setup downloads CEF and the build actually contains it. The
# Windows presets already set this; a plain cmake configure did not.
if(OS_WINDOWS AND NOT DEFINED ENABLE_BROWSER)
  set(ENABLE_BROWSER ON CACHE BOOL "Enable browser sources (requires CEF)")
endif()

include(compilerconfig)
include(defaults)
include(helpers)

option(ENABLE_FRONTEND "Enable building with UI (requires Qt)" ON)
option(ENABLE_SCRIPTING "Enable scripting support" ON)
option(ENABLE_HEVC "Enable HEVC encoders" ON)

add_subdirectory(libobs)
if(OS_WINDOWS)
  add_subdirectory(libobs-d3d11)
  add_subdirectory(libobs-winrt)
endif()
add_subdirectory(libobs-opengl)
if(OS_MACOS)
  add_subdirectory(libobs-metal)
endif()
add_subdirectory(plugins)

add_subdirectory(test/test-input)

add_subdirectory(frontend)

message_configuration()
