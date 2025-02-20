cmake_minimum_required(VERSION 3.20)

set(TARGET lv_font_conv)

project(${TARGET})

find_package(Freetype REQUIRED)
include_directories(
    ${FREETYPE_INCLUDE_DIRS}
    
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/res
    )
add_subdirectory(src)

add_executable(${TARGET} main.cpp)
target_link_libraries(
    ${TARGET}
    -Wl,--start-group
    source
    pthread
    freetype
    m
    -Wl,--end-group
)

install(TARGETS lv_font_conv DESTINATION bin)