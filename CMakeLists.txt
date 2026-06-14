cmake_minimum_required(VERSION 3.20)
project(mybilibili-obs-plugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Dirs from workflow env vars (or default fallbacks)
if(DEFINED ENV{OBS_SRC_DIR})
  set(OBS_SRC_DIR $ENV{OBS_SRC_DIR})
endif()
if(DEFINED ENV{OBS_DEPS_DIR})
  set(OBS_DEPS_DIR $ENV{OBS_DEPS_DIR})
endif()
if(DEFINED ENV{QT_DIR})
  set(QT_DIR $ENV{QT_DIR})
endif()

# ── Qt6 ──────────────────────────────────────────
set(CMAKE_PREFIX_PATH "${QT_DIR}" ${CMAKE_PREFIX_PATH})
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

# ── libobs ────────────────────────────────────────
find_path(OBS_INCLUDE_DIR
  NAMES obs.h obs.hpp
  PATHS "${OBS_SRC_DIR}/libobs"
  NO_DEFAULT_PATH
  REQUIRED
)
find_library(OBS_LIBRARY
  NAMES obs libobs
  PATHS "${OBS_DEPS_DIR}/lib"
  NO_DEFAULT_PATH
  REQUIRED
)

# ── obs-frontend-api ─────────────────────────────
find_path(OBS_FRONTEND_INCLUDE_DIR
  NAMES obs-frontend-api.h
  PATHS "${OBS_SRC_DIR}/UI/obs-frontend-api"
  NO_DEFAULT_PATH
  REQUIRED
)
find_library(OBS_FRONTEND_LIBRARY
  NAMES obs-frontend-api
  PATHS "${OBS_DEPS_DIR}/lib"
  NO_DEFAULT_PATH
  REQUIRED
)

# ── CURL ──────────────────────────────────────────
find_library(CURL_LIBRARY
  NAMES curl libcurl
  PATHS "${OBS_DEPS_DIR}/lib"
  NO_DEFAULT_PATH
  REQUIRED
)
find_path(CURL_INCLUDE_DIR
  NAMES curl/curl.h
  PATHS "${OBS_DEPS_DIR}/include"
  NO_DEFAULT_PATH
  REQUIRED
)

# ── Git hash ─────────────────────────────────────
execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(NOT GIT_COMMIT_HASH)
  set(GIT_COMMIT_HASH "unknown")
endif()

# ── Plugin target ────────────────────────────────
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
  "${OBS_INCLUDE_DIR}"
  "${OBS_FRONTEND_INCLUDE_DIR}"
  "${CURL_INCLUDE_DIR}"
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  "${OBS_LIBRARY}"
  "${OBS_FRONTEND_LIBRARY}"
  "${CURL_LIBRARY}"
  Qt6::Core
  Qt6::Widgets
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
  PLUGIN_VERSION_STR="${PROJECT_VERSION}"
  GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
)

if(MSVC)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W0)
endif()

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
  PREFIX ""
  SUFFIX ".dll"
)