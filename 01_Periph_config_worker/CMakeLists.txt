cmake_minimum_required(VERSION 3.7)

project(peripherals_config_worker)

set(
	compiller_options -Wall -std=c++17 # -g -O0
)

set(INC_PATH "include")
set(SRC_PATH "src")

file(GLOB HEADERS ${INC_PATH}/*.h)
file(GLOB SOURCES ${SRC_PATH}/*.cpp)

include_directories(${INC_PATH})
include_directories(../00_Common_stuff/include/)

add_executable(${PROJECT_NAME}_test
	${SOURCES} 
	${HEADERS}
	main.cpp
)

add_library(${PROJECT_NAME} STATIC 	
	${SOURCES} 
	${HEADERS}
)

target_compile_options(${PROJECT_NAME}_test PRIVATE ${compiller_options})
target_link_libraries(${PROJECT_NAME}_test PUBLIC stdc++fs)
target_link_libraries(${PROJECT_NAME} PUBLIC stdc++fs)