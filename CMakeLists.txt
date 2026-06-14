cmake_minimum_required(VERSION 3.20)
project(mybilibili-obs-plugin)

# ── Bootstrap the cmake/common build system ─────────────
# Sets up: CMAKE_MODULE_PATH, OS detection, build types,
# PLUGIN_VERSION from buildspec.json, etc.
include(cmake/common/bootstrap.cmake)

# ── OS-specific defaults + dependency download ──────────
# On Windows: includes cmake/windows/buildspec.cmake which
# calls _check_dependencies_windows() → downloads:
#   - obs-studio source (for libobs headers & lib)
#   - obs-deps (prebuilt 3rd-party libs: ffmpeg, curl, ...)
#   - Qt6
# Then compiles libobs + obs-frontend-api from source.
include(defaults)

# ── Git commit hash ──────────────────────────────────────
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE _git_hash
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(NOT _git_hash)
  set(_git_hash "unknown")
endif()

# ── Find OBS packages ──────────────────────────────────
# After _setup_obs_studio(), the OBS Development component
# is installed to CMAKE_PREFIX_PATH with cmake config files.
find_package(LibObs REQUIRED)
find_package(obs-frontend-api REQUIRED)

# ── Qt6 (from obs-deps-qt6) ─────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

# ── CURL (from obs-deps) ────────────────────────────────
find_path(CURL_INCLUDE_DIR curl/curl.h)
find_library(CURL_LIBRARY NAMES curl libcurl)
if(NOT CURL_INCLUDE_DIR OR NOT CURL_LIBRARY)
  message(FATAL_ERROR "CURL not found in CMAKE_PREFIX_PATH (check obs-deps)")
endif()

# ── Plugin library ──────────────────────────────────────
add_library(${CMAKE_PROJECT_NAME} MODULE
  src/mybili_api.hpp
  src/http_client.hpp
  src/plugin_utils.hpp
  src/md5.hpp
  src/qrcodegen/qrcodegen.hpp
  src/json11/json11.hpp
  src/core/config_manager.hpp
  src/core/qr_generator.hpp
  src/ui/menu_manager.hpp
  src/ui/dialog_factory.hpp
  src/plugin-main.cpp
  src/mybili_api.cpp
  src/http_client.cpp
  src/plugin_utils.cpp
  src/md5.cpp
  src/qrcodegen/qrcodegen.cpp
  src/json11/json11.cpp
  src/core/config_manager.cpp
  src/core/qr_generator.cpp
  src/ui/menu_manager.cpp
  src/ui/dialog_factory.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  src
  ${CURL_INCLUDE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Widgets
  Qt6::Core
  ${CURL_LIBRARY}
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
  PLUGIN_VERSION_STR="${PLUGIN_VERSION}"
  GIT_COMMIT_HASH="${_git_hash}"
)

# ── Platform tweaks ─────────────────────────────────────
if(MSVC)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W0)
endif()

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
  PREFIX ""
  SUFFIX ".dll"
)