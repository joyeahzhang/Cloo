cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(my_project_name VERSION 1.0 LANGUAGES CXX)

# 设置C++版本
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 导出Complie_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 包含头文件路径
include_directories(net/include)

# 把net目录下的所有源文件添加到变量SRC_LIST中
aux_source_directory(net SRC_LIST)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
# 设置输出测试目录
set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/build/test)

# 从测试文件夹编译测试用例
file(GLOB_RECURSE TEST_SRC_LIST test/*.cc)

foreach(test_src_file ${TEST_SRC_LIST})
    get_filename_component(test_name ${test_src_file} NAME_WE)
    add_executable(${test_name} ${test_src_file} ${SRC_LIST})
    set_target_properties(${test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEST_OUTPUT_DIR})
    add_custom_target(${test_name}_build
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${test_name}
        COMMENT "Building ${test_name}")
endforeach()