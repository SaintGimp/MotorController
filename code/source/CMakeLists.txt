target_sources(MotorController    
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/version.h"
)

target_include_directories(MotorController PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

include(${CMAKE_CURRENT_LIST_DIR}/servo_controller/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/console/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/systick/CMakeLists.txt)
