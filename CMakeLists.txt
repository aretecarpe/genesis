cmake_minimum_required(VERSION 3.21)

set(CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake") # we are going to assume the cmake is here.

message("CMAKE_PATH: ${CMAKE_PATH}")

include("${CMAKE_PATH}/project-template.cmake")

project(${PROJECT_NAME}
	VERSION ${PROJECT_VERSION}
	DESCRIPTION ${PROJECT_DESCRIPTION}
	LANGUAGES CXX
)

foreach(product_path ${PRODUCT_LIST})
	set(PRODUCT_SUBDIRECTORY ${product_path})
	add_subdirectory(${product_path})
endforeach()