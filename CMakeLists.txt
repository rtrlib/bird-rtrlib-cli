project(bird-rtrlib-cli)
cmake_minimum_required(VERSION 2.6)
add_executable(bird-rtrlib-cli bird-rtrlib-cli.c bird.c cli.c config.c util.c)
