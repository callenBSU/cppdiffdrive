# CMake generated Testfile for 
# Source directory: /home/vboxuser/cppdiffdrive/src/diff_drive
# Build directory: /home/vboxuser/cppdiffdrive/build/diff_drive
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(test_motor_interface "/usr/bin/python3" "-u" "/opt/ros/jazzy/share/ament_cmake_test/cmake/run_test.py" "/home/vboxuser/cppdiffdrive/build/diff_drive/test_results/diff_drive/test_motor_interface.gtest.xml" "--package-name" "diff_drive" "--output-file" "/home/vboxuser/cppdiffdrive/build/diff_drive/ament_cmake_gtest/test_motor_interface.txt" "--command" "/home/vboxuser/cppdiffdrive/build/diff_drive/test_motor_interface" "--gtest_output=xml:/home/vboxuser/cppdiffdrive/build/diff_drive/test_results/diff_drive/test_motor_interface.gtest.xml")
set_tests_properties(test_motor_interface PROPERTIES  LABELS "gtest" REQUIRED_FILES "/home/vboxuser/cppdiffdrive/build/diff_drive/test_motor_interface" TIMEOUT "60" WORKING_DIRECTORY "/home/vboxuser/cppdiffdrive/build/diff_drive" _BACKTRACE_TRIPLES "/opt/ros/jazzy/share/ament_cmake_test/cmake/ament_add_test.cmake;125;add_test;/opt/ros/jazzy/share/ament_cmake_gtest/cmake/ament_add_gtest_test.cmake;95;ament_add_test;/opt/ros/jazzy/share/ament_cmake_gtest/cmake/ament_add_gtest.cmake;93;ament_add_gtest_test;/home/vboxuser/cppdiffdrive/src/diff_drive/CMakeLists.txt;50;ament_add_gtest;/home/vboxuser/cppdiffdrive/src/diff_drive/CMakeLists.txt;0;")
subdirs("gtest")
