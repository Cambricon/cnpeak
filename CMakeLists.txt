cmake_minimum_required (VERSION 2.6)
project (cnpipe3)

set(OS_VERSION "ubuntu_1804")
#set(OS_VERSION "centos")

#compile app
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/app/app_yolov5")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/app/app_openpose")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/app/app_ocr")