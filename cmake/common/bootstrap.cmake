# OBS CMake bootstrap module

include_guard(GLOBAL)

# Map fallback configurations for optimized build configurations
# gersemi: off
set(
  CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO
    RelWithDebInfo
    Release
    MinSizeRel
    None
    ""
)
set(
  CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL
    MinSizeRel
    Release
    RelWithDebInfo
    None
    ""
)
set(
  CMAKE_MAP_IMPORTED_CONFIG_RELEASE
    Release
    RelWithDebInfo
    MinSizeRel
    None
    ""
)
# gersemi: on

# Prohibit in-source builds
if("${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  message(
    FATAL_ERROR
    "In-source builds of OBS are not supported. "
    "Specify a build directory via 'cmake -S <SOURCE DIRECTORY> -B <BUILD_DIRECTORY>' instead."
  )
  file(REMOVE_RECURSE "${CMAKE_CURRENT_SOURCE_DIR}/CMakeCache.txt" "${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles")
endif()

# Set default global project variables
set(OBS_COMPANY_NAME "XStream")
set(OBS_PRODUCT_NAME "XStream")
set(OBS_WEBSITE "https://www.obsproject.com")
set(
  OBS_COMMENTS
  "Free and open source software for video recording and live streaming. Based on OBS Studio by the OBS Project."
)
set(OBS_LEGAL_COPYRIGHT "(C) Lain Bailey")
set(OBS_CMAKE_VERSION 3.0.0)

# Configure default version strings. base-version.txt is the upstream OBS
# Studio release this tree is based on. A fork checkout has no OBS version
# tags, so git describe cannot produce MAJOR.MINOR.PATCH on its own.
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/base-version.txt" _obs_base_version_line LIMIT_COUNT 1)
string(STRIP "${_obs_base_version_line}" _obs_base_version_line)
string(REPLACE "." ";" _obs_default_version "${_obs_base_version_line}")
set(_obs_release_candidate 0)
set(_obs_beta 0)

# Add common module directories to default search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/finders")

include(policies NO_POLICY_SCOPE)
include(versionconfig)
include(buildnumber)
include(osconfig)

# Allow selection of common build types via UI
if(NOT CMAKE_GENERATOR MATCHES "(Xcode|Visual Studio .+)")
  if(NOT CMAKE_BUILD_TYPE)
    set(
      CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING
      "OBS build type [Release, RelWithDebInfo, Debug, MinSizeRel]"
      FORCE
    )
    set_property(
      CACHE CMAKE_BUILD_TYPE
      PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel
    )
  endif()
endif()

# Enable default inclusion of targets' source and binary directory
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
