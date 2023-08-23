cmake_minimum_required(VERSION 3.12.4)

set(TARGET lv_font_conv)

project(${TARGET})

aux_source_directory(. SRCS)

include_directories(. include /usr/local/include/freetype2)

add_subdirectory(source)

add_executable(${TARGET} ${SRCS})

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