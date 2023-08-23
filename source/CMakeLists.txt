cmake_minimum_required(VERSION 3.12.4)

aux_source_directory(. SOURCE)
add_library(source ${SOURCE})

add_compile_options(-fshort-wchar)

include_directories(. ../include /usr/local/include/freetype2)
