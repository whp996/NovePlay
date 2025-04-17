# 设置最低 CMake 版本
cmake_minimum_required(VERSION 3.10)
# 定义项目名称
project(ChatServe)
# 使用 C++11 标准
set(CMAKE_CXX_STANDARD 11)
# 添加静态库
add_library(ChatServe STATIC serve.cpp)
# 链接 SQLite3 库替换 mysqlclient
target_link_libraries(ChatServe PUBLIC sqlite3)
