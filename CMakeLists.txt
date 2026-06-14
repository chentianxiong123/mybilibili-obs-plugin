cmake_minimum_required(VERSION 3.20)
project(mybilibili-obs-plugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(libobs REQUIRED)
find_package(CURL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

# Get the git commit hash
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

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE src)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  OBS::libobs
  CURL::libcurl
  Qt6::Core
  Qt6::Widgets
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
  PLUGIN_VERSION_STR="${PROJECT_VERSION}"
  GIT_COMMIT_HASH="${GIT_COMMIT_HASH}"
)

# Suppress specific warnings on MSVC
if(MSVC)
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W0)
endif()

# Set output name
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
  PREFIX ""
  SUFFIX ".dll"
)

install(TARGETS ${CMAKE_PROJECT_NAME}
  LIBRARY DESTINATION obs-plugins/64bit
)