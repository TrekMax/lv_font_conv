aux_source_directory(. SOURCE)
add_library(source ${SOURCE})

add_compile_options(-fshort-wchar)
