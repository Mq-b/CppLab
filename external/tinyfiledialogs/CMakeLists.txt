add_library(tinyfiledialogs STATIC
    tinyfiledialogs.c
)

target_include_directories(tinyfiledialogs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})