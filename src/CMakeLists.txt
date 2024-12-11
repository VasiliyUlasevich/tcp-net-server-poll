cmake_minimum_required(VERSION 3.24)

project(
  tcp-net-server-poll
  VERSION 0.1
  LANGUAGES C)

add_library(tcp_server tcp_server.c tcp_server.h)

add_executable(Server main.c)

target_link_libraries(Server PRIVATE tcp_server)
