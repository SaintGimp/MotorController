target_sources(MotorController
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/systick.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/systick.h"
)

target_include_directories(MotorController PUBLIC "${CMAKE_CURRENT_LIST_DIR}")