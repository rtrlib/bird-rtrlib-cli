project(bird-rtrlib-cli)
cmake_minimum_required(VERSION 2.6)
add_executable(bird-rtrlib-cli bird-rtrlib-cli.c bird.c rtr.c cli.c config.c)

include_directories(${RTRLIB_INCLUDE} ${INCLUDE_DIRECTORIES})

target_link_libraries(bird-rtrlib-cli rtr)
