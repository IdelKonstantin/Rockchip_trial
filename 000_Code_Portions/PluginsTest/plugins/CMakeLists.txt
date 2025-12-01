set(
	compiller_options -Wall -std=c++17 -g -O0
)

# Плагин 1
add_library(plugin1 SHARED plugin1.cpp)
target_include_directories(plugin1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Плагин 2  
add_library(plugin2 SHARED plugin2.cpp)
target_include_directories(plugin2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Копируем плагины в отдельную директорию после сборки
add_custom_command(TARGET plugin1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/plugins
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:plugin1> ${CMAKE_BINARY_DIR}/plugins/
)

add_custom_command(TARGET plugin2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:plugin2> ${CMAKE_BINARY_DIR}/plugins/
)