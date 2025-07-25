cmake_minimum_required(VERSION 3.10)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ProjectName "MD-Sign")
project(${ProjectName})
set(CMAKE_INSTALL_PREFIX /usr/local)
set(version 0.1a)

add_library(mdfs-common STATIC
    #headers
    include/common/mbr.hpp
    include/common/gpt.hpp
    include/common/units.hpp
    include/common/crc32.hpp
    include/common/guid.hpp
    #sources
    src/common/mbr.cpp
    src/common/gpt.cpp
    src/common/crc32.cpp
    src/common/guid.cpp
)

target_include_directories(mdfs-common PUBLIC include)

add_executable(mdfs-partinit
    #headers

    #sources
    src/partinit.cpp
)

target_include_directories(mdfs-partinit PUBLIC include)
target_link_libraries(mdfs-partinit PRIVATE mdfs-common)
target_compile_definitions(mdfs-partinit PUBLIC VERSION="${version}")

add_executable(mdfs-guidtoarr 
    src/guid_to_arr.cpp
)
target_link_libraries(mdfs-guidtoarr PRIVATE mdfs-common)
install(TARGETS mdfs-guidtoarr RUNTIME DESTINATION bin)
