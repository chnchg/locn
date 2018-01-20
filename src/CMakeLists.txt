cmake_minimum_required(VERSION 3.4)
project(loci)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
include_directories(${PROJECT_SOURCE_DIR})
set(CC_SRC
	tiff.cc tiff.hh
	nelder_mead.cc nelder_mead.hh
	utils.cc utils.hh
)

add_executable(lczn localization.cc ${CC_SRC})
add_executable(loc1 loc1.cc tiff.cc tiff.hh
	nelder_mead.hxx utils.hxx utils.cc utils.hh)
