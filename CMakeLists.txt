cmake_minimum_required(VERSION 3.15)

# Tell CMake to allow older subprojects (like nlohmann_json) to configure without error
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

project(GhostServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# We will use FetchContent to download header-only or CMake-friendly libraries
# directly during the CMake configuration step, making it package-manager agnostic.
include(FetchContent)

# 1. Crow (C++ Microframework for web)
FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG        v1.2.0 # Or whichever version you prefer
)
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(Crow)

# We create a dummy nlohmann folder structure so `#include <nlohmann/json.hpp>` works
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/nlohmann)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/json.hpp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/nlohmann)

# To prevent jwt-cpp from fetching its own nlohmann_json, we tell CMake that it's already found!
# We just use the json.hpp that is already in your source folder.
add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
target_include_directories(nlohmann_json::nlohmann_json INTERFACE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
set(nlohmann_json_FOUND TRUE)

# 2. JWT-CPP
FetchContent_Declare(
  jwt-cpp
  GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
  GIT_TAG        v0.7.0
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JWT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(jwt-cpp)

# 3. System Dependencies
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# Add all the source files
add_executable(GhostServer
    GhostServer.cpp
    api.cpp
    DatabaseHandler.cpp
    LoginManager.cpp
    CppSQLite3.cpp
)

target_include_directories(GhostServer PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(GhostServer PRIVATE
    SQLite::SQLite3
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    Crow::Crow
    jwt-cpp::jwt-cpp
)

if(WIN32)
    target_link_libraries(GhostServer PRIVATE ws2_32 mswsock)
endif()