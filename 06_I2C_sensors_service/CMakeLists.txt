cmake_minimum_required(VERSION 3.7)

project(i2c_daemon)

set(
	compiller_options -Wall -std=c++17 -g -O0
)

set(INC_PATH "include")
set(SRC_PATH "src")

file(GLOB HEADERS ${INC_PATH}/*.h)

# Рекурсивный поиск всех .cpp файлов в src и подкаталогах
file(GLOB_RECURSE SOURCES 
    ${SRC_PATH}/*.cpp
    ${SRC_PATH}/**/*.cpp
)

# Также рекурсивно для заголовочных файлов если нужно
file(GLOB_RECURSE HEADERS_RECURSIVE
    ${INC_PATH}/*.h
    ${INC_PATH}/**/*.h
)

include_directories(${INC_PATH})
include_directories(${INC_PATH}/imu/)
include_directories(${INC_PATH}/imu/MPU6050)
include_directories(${INC_PATH}/imu/ICM20948)
include_directories(${INC_PATH}/lux/)
include_directories(${INC_PATH}/proxy/)
include_directories(${INC_PATH}/meteo/)
include_directories(../00_Common_stuff/include/)

link_directories(${PROJECT_NAME} ../00_Common_stuff)

# Вывод информации о найденных файлах для отладки
message(STATUS "Found source files:")
foreach(source ${SOURCES})
    message(STATUS "  ${source}")
endforeach()

message(STATUS "Found header files:")
foreach(header ${HEADERS_RECURSIVE})
    message(STATUS "  ${header}")
endforeach()

add_executable(${PROJECT_NAME}
	${SOURCES} 
	${HEADERS_RECURSIVE}
	main.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE ${compiller_options})

target_link_libraries(${PROJECT_NAME} PUBLIC
	stdc++fs
	common
	pthread
	zmq
)

#target_compile_options(${PROJECT_NAME}_tester PRIVATE ${compiller_options})

#add_executable(${PROJECT_NAME}_tester
#	${SOURCES} 
#	${HEADERS}
#	push_and_sub_client.cpp
#)

#target_link_libraries(${PROJECT_NAME}_tester PUBLIC
#	stdc++fs
#	common
#	pthread
#	zmq
#)