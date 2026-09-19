cmake_minimum_required(VERSION 3.22)
project(SCT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(external/Dobby)

add_library(SCT SHARED
    src/main.cpp
    src/pvz2/PassDamageArmor/PassDamageArmor.cpp
    src/pvz2/PassDamageArmor/ArmorPropertySheet/PassDamageArmorPropertySheet.cpp
    src/pvz2/PassDamageArmor/Armor/ApplyDamageHook.cpp
    src/pvz2/PassDamageArmor/Armor/NewspaperArmorHook.cpp
    src/pvz2/PassDamageArmor/Combat/ProjectileCollisionHook.cpp
    src/memory/memory.cpp
    src/memory/initarray.cpp
    src/memory/abortrecovery.cpp
)

target_include_directories(SCT PRIVATE
    src
    external/Dobby/include
)

target_compile_options(SCT PRIVATE
    -Wall
    -Wextra
)

target_link_libraries(SCT
    dobby
    log
    android
    dl
)
