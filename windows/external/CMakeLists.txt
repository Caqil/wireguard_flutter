cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
  base64
  GIT_REPOSITORY https://github.com/aklomp/base64
  GIT_TAG v0.5.0
)

FetchContent_GetProperties(base64)
if(NOT base64_POPULATED)
  FetchContent_Populate(base64)
  add_subdirectory(${base64_SOURCE_DIR} ${base64_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()