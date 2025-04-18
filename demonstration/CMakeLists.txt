cmake_minimum_required(VERSION 3.10.0)

project(demonstration VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)

set(Chapter
    demonstration
    Chapter0
    Chapter1
)

foreach(chapter ${Chapter})
add_executable(${chapter} src/${chapter}.cpp)
target_link_directories(
    ${chapter}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/Panda3D-1.10.14-x64/lib
)

#libp3framework.lib
#libpanda.lib
#libpandaexpress.lib
#libp3dtool.lib
#libp3dtoolconfig.lib
#libp3direct.lib
#libpandafx.lib
#libpandaphysics.lib

target_link_libraries(
    ${chapter}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libp3framework.lib
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libpanda.lib
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libpandaexpress.lib
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libp3dtool.lib
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libp3dtoolconfig.lib
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libp3direct.lib
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libpandafx.lib
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib/libpandaphysics.lib
)
endforeach(chapter ${Chapter})

include_directories(
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/include
)

link_directories(
    ${CMAKE_SOURCE_DIR}/../Panda3D-1.10.14-x64/lib
)