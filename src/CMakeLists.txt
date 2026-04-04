find_package(CURL REQUIRED)

add_executable(dump candump.c)
add_executable(send cansend.c)
add_executable(canClient canClient.c)

target_link_libraries(canClient PRIVATE CURL::libcurl)
target_include_directories(canClient PRIVATE ${CURL_INCLUDE_DIRS})