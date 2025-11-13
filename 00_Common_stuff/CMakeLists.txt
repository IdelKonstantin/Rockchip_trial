cmake_minimum_required(VERSION 3.7)

project(common)

set(
	compiller_options -Wall -std=c++17 -g -O0
)

set(INC_PATH "include")
set(SRC_PATH "src")

file(GLOB HEADERS ${INC_PATH}/*.h)
file(GLOB SOURCES ${SRC_PATH}/*.cpp)

include_directories(${INC_PATH})

add_library(${PROJECT_NAME} STATIC 	
	${SOURCES} 
	${HEADERS}
)

target_compile_options(${PROJECT_NAME} PRIVATE ${compiller_options})

target_link_libraries(${PROJECT_NAME} PUBLIC
	stdc++fs
	pthread
)