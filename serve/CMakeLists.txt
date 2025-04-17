cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ChatServe")

# 包含子项目。
add_subdirectory ("Serve")

# 添加包含 main 函数的可执行文件，并链接 ChatServe 静态库
add_executable(ChatServeApp main.cpp)
target_link_libraries(ChatServeApp PRIVATE ChatServe)
target_include_directories(ChatServeApp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Serve)