# The Flutter tooling requires that developers have a version of Visual Studio
# installed that includes CMake 3.14 or later. You should not increase this
# version, as doing so will cause the plugin to fail to compile for some
# customers of the plugin.
cmake_minimum_required(VERSION 3.14)

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

# Project-level configuration.
set(PROJECT_NAME "wireguard_dart")
project(${PROJECT_NAME} LANGUAGES CXX)

# This value is used when generating builds using this plugin, so it must
# not be changed
set(PLUGIN_NAME "wireguard_dart_plugin")

# Any new source files that you add to the plugin should be added here.
list(APPEND PLUGIN_SOURCES
  "wireguard_dart_plugin.cpp"
  "wireguard_dart_plugin.h"
  "key_generator.cpp"
  "key_generator.h"
  "config_writer.cpp"
  "config_writer.h"
  "service_control.cpp"
  "service_control.h"
  "utils.cpp"
  "utils.h"
)

# Define the plugin library target. Its name must not be changed (see comment
# on PLUGIN_NAME above).
add_library(${PLUGIN_NAME} SHARED
  "include/wireguard_dart/wireguard_dart_plugin_c_api.h"
  "wireguard_dart_plugin_c_api.cpp"
  ${PLUGIN_SOURCES}
)

# Apply a standard set of build settings that are configured in the
# application-level CMakeLists.txt. This can be removed for plugins that want
# full control over build settings.
apply_standard_settings(${PLUGIN_NAME})

# Symbols are hidden by default to reduce the chance of accidental conflicts
# between plugins. This should not be removed; any symbols that should be
# exported should be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

# Source include directories and library dependencies. Add any plugin-specific
# dependencies here.
add_subdirectory(external)
target_link_libraries(${PLUGIN_NAME} PRIVATE base64)

add_compile_definitions(WIN32_LEAN_AND_MEAN) # for Wireguard winsock/windows conflict

add_library(tunnel SHARED IMPORTED GLOBAL)
set_target_properties(tunnel PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/tunnel/amd64/tunnel.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/tunnel/amd64/tunnel.lib"
)
target_include_directories(tunnel INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/tunnel/include")
target_link_libraries(${PLUGIN_NAME} PRIVATE tunnel)

add_library(wireguard SHARED IMPORTED GLOBAL)
set_target_properties(wireguard PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/wireguard/amd64/wireguard.dll"
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/wireguard/amd64/wireguard.lib"
)
target_include_directories(wireguard INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/wireguard/include")
target_link_libraries(${PLUGIN_NAME} PRIVATE wireguard)

target_include_directories(${PLUGIN_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/tunnel/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/wireguard/include"
)
target_link_libraries(${PLUGIN_NAME} PRIVATE flutter flutter_wrapper_plugin)

# List of absolute paths to libraries that should be bundled with the plugin.
# This list could contain prebuilt libraries, or libraries created by an
# external build triggered from this build file.
set(wireguard_dart_bundled_libraries
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/wireguard/amd64/wireguard.dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/tunnel/amd64/tunnel.dll"
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/wireguard_svc/amd64/wireguard_svc.exe"
  PARENT_SCOPE
)
