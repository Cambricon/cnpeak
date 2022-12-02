cmake_minimum_required (VERSION 2.6)
project (app_openpose)

# ==================== set var ===========================
set(CMAKE_BUILD_TYPE "Debug")

if(${OS_VERSION} STREQUAL "ubuntu_1804")
    set(CMAKE_CXX_FLAGS_DEBUG "-std=c++14 -Wall -Werror=return-type ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "-std=c++14 ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS "-lpthread -fPIC ${CMAKE_EXE_LINKER_FLAGS}")
    set(CUR_OS          "ubuntu")
elseif(${OS_VERSION} STREQUAL "ubuntu_1604")
    set(CMAKE_CXX_FLAGS_DEBUG "-std=c++14 -Wall -Werror=return-type ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "-std=c++14 ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS "-lpthread -fPIC ${CMAKE_EXE_LINKER_FLAGS}")
    set(CUR_OS          "ubuntu")
elseif(${OS_VERSION} STREQUAL "centos")
    set(CMAKE_CXX_FLAGS_DEBUG "-std=c++11 -Wall -Werror=return-type -D_GLIBCXX_USE_CXX11_ABI=0 -O0 ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 -D_GLIBCXX_USE_CXX11_ABI=0 -O0 ${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS "-lpthread -fPIC ${CMAKE_EXE_LINKER_FLAGS}")
    set(CUR_OS          "centos")
endif()

# magicmind need to disable abi, if enable, compile not pass
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)

set(ROOT_DIR        "${CMAKE_CURRENT_LIST_DIR}/../..")
set(3rdPARTY_DIR    "${CMAKE_CURRENT_LIST_DIR}/../../3rdparty")
set(PLUGIN_DIR      "${CMAKE_CURRENT_LIST_DIR}/../../plugin")
set(NEUWARE_HOME    "/usr/local/neuware")

# ==================== include dir ==========================
include_directories(${ROOT_DIR})
include_directories(${3rdPARTY_DIR}/gflag/include/${CUR_OS})
include_directories(${3rdPARTY_DIR}/glog/include)
# include_directories(${3rdPARTY_DIR}/opencv/include)
include_directories(${NEUWARE_HOME}/include)
include_directories(${3rdPARTY_DIR}/ffmpeg/include)
include_directories(${3rdPARTY_DIR}/live555/include)
include_directories(${3rdPARTY_DIR}/live555/include/groupsock)
include_directories(${3rdPARTY_DIR}/live555/include/UsageEnvironment)
include_directories(${3rdPARTY_DIR}/rapidjson/include)

# ==================== link dir ===========================
link_directories(${3rdPARTY_DIR}/gflag/lib/${CUR_OS})
link_directories(${3rdPARTY_DIR}/glog/lib/${CUR_OS})
# link_directories(${3rdPARTY_DIR}/opencv/lib/${CUR_OS})
link_directories(${NEUWARE_HOME}/lib64)
link_directories(${3rdPARTY_DIR}/ffmpeg/lib/${CUR_OS})
link_directories(${3rdPARTY_DIR}/live555/lib/${CUR_OS})

# ==================== link so ===========================
link_libraries("libglog.so")
link_libraries("libgflags.so")
link_libraries("libopencv_core.so")
# link_libraries("libopencv_highgui.so")
# link_libraries("libopencv_imgproc.so")
# link_libraries("libopencv_imgcodecs.so")
link_libraries("libavformat.so")
link_libraries("libavcodec.so")
link_libraries("libavutil.so")
link_libraries("libswresample.so")
link_libraries("libavfilter.so")
link_libraries("libswscale.so")
link_libraries("libcnrt.so")
link_libraries("libcndrv.so")
link_libraries("libcncv.so")
link_libraries("libcncodec_v3.so")
link_libraries("libmagicmind.so")
link_libraries("libmagicmind_runtime.so")
link_libraries("libmagicmind_parser.so")

# find_package(Boost COMPONENTS system filesystem REQUIRED)
# find_package(SDL2 REQUIRED sdl2)
# string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIR})
set(OpenCV_LIBS ${OpenCV_LIBRARIES})

# find_package(FFmpeg REQUIRED)
# include_directories(${FFMPEG_INCLUDE_DIR})
# set(FFMPEG_LIBS ${FFMPEG_LIBRARIES})

# ==================== source ===========================
aux_source_directory(${ROOT_DIR}/core src_core)
set(plugins
        plugin_src_image_cncodec
        plugin_src_video_cncodec
        plugin_id_gen
        plugin_pre_cncv_rc
        plugin_pre_cncv_common
        common_infer
        plugin_infer
        plugin_infer_copyout
        plugin_post_openpose
        # plugin_video_play
        plugin_image_save_opencv
        plugin_video_http)

set(src_plugin)
foreach (p ${plugins})
    file(GLOB_RECURSE f "${PLUGIN_DIR}/${p}/*.cpp")
    list(APPEND src_plugin ${f})
endforeach()

if(${CUR_OS} STREQUAL "ubuntu")
    add_definitions(-DUBUNTU_OS)
    link_libraries("/lib/x86_64-linux-gnu/libpthread.so.0")
elseif(${CUR_OS} STREQUAL "centos")
    add_definitions(-DCENTOS_OS)
endif()

# ==================== bin ===========================
add_executable(app_openpose
        app_openpose.cpp
        ${src_core}
        ${src_plugin})

target_link_libraries(app_openpose
                    # ${SDL2_LIBRARIES} 
                    ${OpenCV_LIBS})
