find_package(CURL REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(dump candump.c ../include//canTypes.c)
add_executable(send cansend.c ../include//canTypes.c)
add_executable(sniffer canSniffer.c ../include/oledi2c.c ../include//canTypes.c)
add_executable(canClient canClient.c ../include//canTypes.c)

target_link_libraries(canClient PRIVATE CURL::libcurl)
target_include_directories(canClient PRIVATE ${CURL_INCLUDE_DIRS})