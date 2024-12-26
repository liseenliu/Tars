set(SOURCES "display/display.cc"
            "application.cc"
            "system_info.cc"
            "main.cc")
set(INCLUDE_DIRS "." "display")

# 添加板级公共文件
file(GLOB BOARD_COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/boards/common/*.cc)
list(APPEND SOURCES ${BOARD_COMMON_SOURCES})
list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards/common)

# 在kernel 中定义的菜单，然后在skdconfig中进行的配置给cmake使用
if(CONFIG_BOARD_TYPE_ESP32S3)
    set(BOARD_TYPE "esp32-s3")
endif()

file(GLOB BOARD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD_TYPE}/*.cc)
list(APPEND SOURCES ${BOARD_SOURCES})

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    )
