cmake_minimum_required(VERSION 3.16)
project(SimpleAIAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SimpleAIAgent WIN32 src/main.cpp)

target_link_libraries(SimpleAIAgent PRIVATE
    comctl32
    winhttp
)

if(MSVC)
    target_compile_options(SimpleAIAgent PRIVATE /W3 /utf-8 /MP /O2 /GL)
    target_link_options(SimpleAIAgent PRIVATE /LTCG)
else()
    target_compile_options(SimpleAIAgent PRIVATE -Wall -O2)
endif()

install(TARGETS SimpleAIAgent RUNTIME DESTINATION bin)
