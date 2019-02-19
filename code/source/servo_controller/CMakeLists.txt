target_sources(MotorController
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/tmc4671_registers.h"
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/tmc4671.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/tmc4671.h"
)

target_include_directories(MotorController PUBLIC "${CMAKE_CURRENT_LIST_DIR}")