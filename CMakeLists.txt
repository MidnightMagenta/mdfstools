cmake_minimum_required(VERSION 3.10)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ProjectName "MD-Sign")
project(${ProjectName})
set(CXX_STANDARD 23)
set(CMAKE_INSTALL_PREFIX /usr/local)
set(version 0.1a)

add_library(mdfs-common STATIC
    #headers
    include/common/mbr.hpp
    include/common/gpt.hpp
    include/common/units.hpp
    include/common/crc32.hpp
    include/common/guid.hpp
    include/common/djb2.hpp
    include/common/result.hpp
    include/common/align.hpp
    #sources
    src/common/mbr.cpp
    src/common/gpt.cpp
    src/common/crc32.cpp
    src/common/guid.cpp
)

target_include_directories(mdfs-common PUBLIC include)

add_executable(mdfst
    #headers
    include/part/init_table.hpp
    #sources
    src/part/main.cpp
    src/part/init_table.cpp
)

target_include_directories(mdfst PUBLIC include)
target_link_libraries(mdfst PRIVATE mdfs-common)
target_compile_definitions(mdfst PUBLIC VERSION="${version}")

add_executable(mdfs-uuidarr 
    src/guid_to_arr.cpp
)
target_link_libraries(mdfs-uuidarr PRIVATE mdfs-common)
install(TARGETS mdfs-uuidarr RUNTIME DESTINATION bin)
