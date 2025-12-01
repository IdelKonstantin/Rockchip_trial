cmake_minimum_required(VERSION 3.10)
project(ImageOverlay)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc highgui)

add_executable(image_overlay main.cpp)
target_link_libraries(
	image_overlay 
	${OpenCV_LIBS}
	gomp
)