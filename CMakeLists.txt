cmake_minimum_required(VERSION 2.8)
project(tinybuf)

#使能c++11
#add_compile_options(-std=c++11)
set(CMAKE_CXX_STANDARD 11)
add_definitions(-Wno-int-to-void-pointer-cast -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast)


#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#设置库文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#设置工程源码根目录
set(Source_Root ${CMAKE_CURRENT_SOURCE_DIR}/src)

#设置头文件目录
INCLUDE_DIRECTORIES(${Source_Root}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/)


#收集源代码
file(GLOB tinybuf_src_Root ${Source_Root}/source/*.c)
file(GLOB json_src_Root ${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/jsoncpp/*.cpp)

add_library(tinybuf STATIC ${tinybuf_src_Root})
add_library(jsoncpp STATIC ${json_src_Root})

add_executable(benchmark benchmark.cpp)

set(LINK_LIST)
find_package(JEMALLOC QUIET)
if(JEMALLOC_FOUND)
    set(LINK_LIST ${JEMALLOC_LIBRARY})
endif()

target_link_libraries(benchmark jsoncpp tinybuf ${LINK_LIST} pthread)






