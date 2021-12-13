cmake_minimum_required(VERSION 3.10)

project(BokutachiHook VERSION 1.0 LANGUAGES CXX)

find_package(nlohmann_json 3.9.0 REQUIRED CONFIG)
find_package(CURL CONFIG REQUIRED)

add_library(BokutachiHook
	SHARED
	src/dllmain.cpp
	src/mem.cpp
)

target_include_directories(
	BokutachiHook
	PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(
    BokutachiHook
    PRIVATE
    cxx_std_14
)

target_link_libraries(BokutachiHook
	PRIVATE
	nlohmann_json::nlohmann_json
	CURL::libcurl
)