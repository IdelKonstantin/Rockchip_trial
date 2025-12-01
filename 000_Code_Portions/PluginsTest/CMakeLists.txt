cmake_minimum_required(VERSION 3.16)
project(PluginSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(
	compiller_options -Wall -std=c++17 -g -O0
)


# Настройка компиляции shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Основное приложение
add_executable(plugin_main main.cpp)
target_include_directories(plugin_main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(plugin_main dl)

# Поддиректория с плагинами
add_subdirectory(plugins)

# Создаем директорию для плагинов в build
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)