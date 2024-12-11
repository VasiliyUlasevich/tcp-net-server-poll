cmake_minimum_required(VERSION 3.24)

project(
  tcp-net-server-poll
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/app)

add_subdirectory(src)
