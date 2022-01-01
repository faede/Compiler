
set(CMAKE_CXX_STANDARD 17)
cmake_minimum_required(VERSION 3.9)
project(toy)

# 添加 libanswer 库目标，STATIC 指定为静态库
add_library(libanswer Paser.cpp Paser.cpp lexer.hpp Paser.hpp)

add_executable(toy toy.cpp)
add_compile_options(-O3 `llvm-config --cxxflags` )

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
