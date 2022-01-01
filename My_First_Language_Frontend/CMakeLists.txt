
set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.9)

#SET (CMAKE_C_COMPILER "/usr/bin/clang") # 填写绝对路径
#SET (CMAKE_CXX_COMPILER "/usr/bin/clang++") # 填写绝对路径
project(toy)

# 添加 libanswer 库目标，STATIC 指定为静态库
add_library(libanswer Paser.cpp Paser.hpp lexer.cpp lexer.hpp IR.cpp IR.hpp)

add_executable(toy toy.cpp)
add_compile_options(-O3 `llvm-config --cxxflags --ldflags --system-libs --libs core` )

# 为 toy 可执行目标链接 libanswer
target_link_libraries(toy libanswer)

#[[
使用如下命令构建本项目：

    cmake -B build      # 生成构建目录
    cmake --build build # 执行构建
    ./build/toy      # 运行 answer 程序

    Ctrl-D quit


    cmake PATH # PATH 是CMakeLists.txt所在目录
#]]


#[[

    for some reason , cmake doesn't work
    using command:
        clang++ -g -O3 toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o toy
    to compile



    When you quit the current demo (by sending an EOF via CTRL+D on Linux or CTRL+Z and ENTER on Windows),
    it dumps out the IR for the entire module generated. 
    Here you can see the big picture with all the functions referencing each other.


#]]