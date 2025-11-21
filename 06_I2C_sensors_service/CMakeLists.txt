cmake_minimum_required(VERSION 3.7)

project(i2c_daemon)

set(
	compiller_options -Wall -std=c++17 -g -O0
)

set(INC_PATH "include")
set(SRC_PATH "src")

file(GLOB HEADERS ${INC_PATH}/*.h)
file(GLOB SOURCES ${SRC_PATH}/*.cpp)

include_directories(${INC_PATH})
include_directories(../00_Common_stuff/include/)

link_directories(${PROJECT_NAME} ../00_Common_stuff)

add_executable(${PROJECT_NAME}
	${SOURCES} 
	${HEADERS}
	main.cpp
)

#add_executable(${PROJECT_NAME}_tester
#	${SOURCES} 
#	${HEADERS}
#	push_and_sub_client.cpp
#)

target_compile_options(${PROJECT_NAME} PRIVATE ${compiller_options})

#target_compile_options(${PROJECT_NAME}_tester PRIVATE ${compiller_options})

target_link_libraries(${PROJECT_NAME} PUBLIC
	stdc++fs
	common
	pthread
	zmq
)

#target_link_libraries(${PROJECT_NAME}_tester PUBLIC
#	stdc++fs
#	common
#	pthread
#	zmq
#)