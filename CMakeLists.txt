cmake_minimum_required(VERSION 3.14)
project(egzamin C)

set(CMAKE_C_STANDARD 99)

add_executable(egzamin main.c old/mydb2.h old/mybaseold.h libs/mylib.c libs/mylib.h RequestProcessor.c RequestProcessor.h ErrorHandling.c ErrorHandling.h libs/mydb.c libs/mydb.h AuthenticationProvider.c AuthenticationProvider.h libs/mybase64.c libs/mybase64.h)
include_directories(libs)