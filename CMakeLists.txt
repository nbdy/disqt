cmake_minimum_required(VERSION 3.2)
project(disqt)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Qml REQUIRED)

add_library(disqt SHARED disqt.h)

set_target_properties(disqt PROPERTIES PUBLIC_HEADER "disqt.h")

target_link_libraries(disqt cpp_redis Qt5::Core Qt5::Qml)

install(TARGETS disqt
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/disqt)