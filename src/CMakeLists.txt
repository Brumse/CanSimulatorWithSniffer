set(LIB_SOURCES
    #i2c_slave_unit.c
    greeting.cpp
)
add_library(cansim_lib STATIC ${LIB_SOURCES})
target_include_directories(cansim_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)
add_executable(hello main.cpp)
target_link_libraries(hello PRIVATE cansim_lib)