cmake_minimum_required(VERSION 3.16)
project(AEZEL VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include paths
include_directories(
    include
    src
)

# Common portable core sources
set(AEZEL_CORE_SOURCES
    src/VehicleMath.cpp
    src/VehicleConfig.cpp
)

# Linux / Raspberry Pi HAL sources
set(AEZEL_LINUX_HAL_SOURCES
    src/hal/linux_pi/LinuxGpio.cpp
    src/hal/linux_pi/LinuxCan.cpp
    src/hal/linux_pi/LinuxStorage.cpp
    src/hal/linux_pi/LinuxBle.cpp
)

# Core static library
add_library(aezel_core STATIC
    ${AEZEL_CORE_SOURCES}
    ${AEZEL_LINUX_HAL_SOURCES}
)

find_package(Threads REQUIRED)
target_link_libraries(aezel_core PUBLIC Threads::Threads)

# Native unit test executables
enable_testing()

add_executable(test_vehicle_math
    test/native/test_vehicle_math.cpp
)
target_link_libraries(test_vehicle_math PRIVATE aezel_core)
add_test(NAME VehicleMathTest COMMAND test_vehicle_math)

add_executable(test_vehicle_profile
    test/native/test_vehicle_profile.cpp
)
target_link_libraries(test_vehicle_profile PRIVATE aezel_core)
add_test(NAME VehicleProfileTest COMMAND test_vehicle_profile)

add_executable(test_hal_linux
    test/native/test_hal_linux.cpp
)
target_link_libraries(test_hal_linux PRIVATE aezel_core)
add_test(NAME LinuxHalTest COMMAND test_hal_linux)
