cmake_minimum_required(VERSION 3.10)

find_package(nlohmann_json 3.9.0 REQUIRED CONFIG)

project(BokutachiLauncher VERSION 1.0 LANGUAGES CXX)

add_executable(BokutachiLauncher
	src/GetAuth.cpp
	src/Injector.cpp
	src/LaunchGame.cpp
	src/Main.cpp
)

target_include_directories(
	BokutachiLauncher
	PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(
    BokutachiLauncher
    PRIVATE
    cxx_std_14
)

target_link_libraries(BokutachiLauncher
	PRIVATE
	nlohmann_json::nlohmann_json
)