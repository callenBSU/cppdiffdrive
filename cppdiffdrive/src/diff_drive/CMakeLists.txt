cmake_minimum_required(VERSION 3.8)
project(diff_drive)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

# from unitree
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

# ── Unitree Motor SDK (only needed for hardware builds) ──────────────────────
# Adjust UNITREE_SDK_PATH to match your install location.
# Comment out this block entirely when building without hardware.
set(UNITREE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(UNITREE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}")
include_directories(${UNITREE_INCLUDE_DIR})
link_directories(${UNITREE_LIB_DIR})

# ── diff_drive_node ──────────────────────────────────────────────────────────
add_executable(diff_drive_node src/diff_drive_node.cpp)
ament_target_dependencies(diff_drive_node rclcpp geometry_msgs std_msgs)

# ── cmd_vel_publisher ────────────────────────────────────────────────────────
add_executable(cmd_vel_publisher src/cmd_vel_publisher.cpp)
ament_target_dependencies(cmd_vel_publisher rclcpp geometry_msgs)

set(UNITREE_LIB libUnitreeMotorSDK.so)

# ── motor_interface_node ─────────────────────────────────────────────────────
add_executable(motor_interface_node src/motor_interface_node.cpp)
target_link_directories(motor_interface_node PRIVATE ${UNITREE_LIB_DIR})
target_link_libraries(motor_interface_node ${UNITREE_LIB})
ament_target_dependencies(motor_interface_node rclcpp std_msgs)

set_target_properties(motor_interface_node PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

target_include_directories(motor_interface_node PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:include>)

install(TARGETS
  diff_drive_node
  cmd_vel_publisher
  motor_interface_node
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY ${UNITREE_LIB_DIR}/
        DESTINATION lib)

# ── Unit tests (no ROS, no SDK required) ─────────────────────────────────────
find_package(ament_cmake_gtest REQUIRED)
ament_add_gtest(test_motor_interface test/test_motor_interface.cpp)

ament_package()
