cmake_minimum_required(VERSION 3.8)
project(diff_drive)

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
set(UNITREE_SDK_PATH "$ENV{HOME}/unitree_motor_sdk")
if(EXISTS "${UNITREE_SDK_PATH}/include")
  include_directories(${UNITREE_SDK_PATH}/include)
  link_directories(${UNITREE_SDK_PATH}/lib)
  add_compile_definitions(UNITREE_SDK_AVAILABLE)
  message(STATUS "Unitree SDK found -- hardware mode enabled")
else()
  message(STATUS "Unitree SDK NOT found -- dry_run only (set dry_run:=true at launch)")
endif()

# ── diff_drive_node ──────────────────────────────────────────────────────────
add_executable(diff_drive_node src/diff_drive_node.cpp)
ament_target_dependencies(diff_drive_node rclcpp geometry_msgs std_msgs)

# ── cmd_vel_publisher ────────────────────────────────────────────────────────
add_executable(cmd_vel_publisher src/cmd_vel_publisher.cpp)
ament_target_dependencies(cmd_vel_publisher rclcpp geometry_msgs)

# ── motor_interface_node ─────────────────────────────────────────────────────
add_executable(motor_interface_node src/motor_interface_node.cpp)
ament_target_dependencies(motor_interface_node rclcpp std_msgs)
if(UNITREE_SDK_AVAILABLE)
  target_link_libraries(motor_interface_node unitreeMotor serialPort)
endif()

install(TARGETS
  diff_drive_node
  cmd_vel_publisher
  motor_interface_node
  DESTINATION lib/${PROJECT_NAME}
)

# ── Unit tests (no ROS, no SDK required) ─────────────────────────────────────
find_package(ament_cmake_gtest REQUIRED)
ament_add_gtest(test_motor_interface test/test_motor_interface.cpp)

ament_package()
