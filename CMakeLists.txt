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
    src/pvz2/UmbrellaLeaf/UmbrellaLeaf.cpp
    src/pvz2/UmbrellaLeaf/Framework/PlantUmbrellaFramework.cpp
    src/pvz2/UmbrellaLeaf/PropertySheet/UmbrellaPropertySheet.cpp
    src/pvz2/UmbrellaLeaf/Anim/UmbrellaAnim.cpp
    src/pvz2/UmbrellaLeaf/Engine/EngineApi.cpp
    src/pvz2/UmbrellaLeaf/Engine/RigApi.cpp
    src/pvz2/UmbrellaLeaf/Engine/EffectApi.cpp
    src/pvz2/UmbrellaLeaf/Bounce/ProjectileBounce.cpp
    src/pvz2/UmbrellaLeaf/Bounce/UmbrellaScan.cpp
    src/pvz2/UmbrellaLeaf/Rig/PlantAnimRigUmbrella.cpp
    src/pvz2/UmbrellaLeaf/Common/Reflection.cpp
    src/pvz2/UmbrellaLeaf/PlantFood/PlantFood.cpp
    src/pvz2/QuestRewardsFix/QuestUnlockPlantIconFix.cpp
    src/pvz2/LawnZoomFix/LawnZoomFix.cpp
    src/memory/memory.cpp
    src/memory/initarray.cpp
    src/memory/abortrecovery.cpp
    src/memory/crashlog.cpp

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
