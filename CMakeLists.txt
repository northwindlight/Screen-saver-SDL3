cmake_minimum_required(VERSION 3.10)

# 如果支持，请为 MSVC 编译器启用热重载。
project("Clock")

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 SDL3 包
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(SDL3_image REQUIRED)

# 将源代码添加到此项目的可执行文件。
add_executable(${PROJECT_NAME} "clock.cpp" "EventThread.h" "EventThread.cpp" "TextureWrapper.hpp")

target_link_libraries(${PROJECT_NAME}
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_image::SDL3_image
)

# TODO: 如有需要，请添加测试并安装目标。
