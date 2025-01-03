set(SOURCES "audio_codecs/audio_codec.cc"
            "audio_codecs/no_audio_codec.cc"
            "audio_processing/audio_processor.cc"
            "audio_processing/wake_word_detect.cc"
            "wifi_connect/wifi_configuration_ap.cc"
            "wifi_connect/wifi_station.cc"
            "display/display.cc"
            "display/no_display.cc"
            "display/ssd1306_display.cc"
            "protocols/protocol.cc"
            "web_socket/web_socket.cc"
            "web_socket/tls_transport.cc"
            "settings.cc"
            "application.cc"
            "system_info.cc"
            "background_task.cc"
            "main.cc")
            
set(INCLUDE_DIRS "." "display" "audio_codecs" "audio_processing")

# 字体
file(GLOB FONT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.c)
list(APPEND SOURCES ${FONT_SOURCES})
list(APPEND INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/fonts)

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

# 在虾哥的工程里 代码是通用级别的，但是暂时先这么抄了。其实这里只需要写在Sources里面就可以了
# if(CONFIG_IDF_TARGET_ESP32S3)
#     list(APPEND SOURCES "audio_processing/audio_processor.cc" "audio_processing/wake_word_detect.cc")
# endif()

idf_component_register(SRCS ${SOURCES}
                    EMBED_FILES "assets/wifi_configuration_ap.html" "assets/favicon.ico" "assets/err_reg.p3" "assets/err_pin.p3" "assets/err_wificonfig.p3"
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    WHOLE_ARCHIVE
                    )

# 使用 target_compile_definitions 来定义 BOARD_TYPE
target_compile_definitions(${COMPONENT_LIB}
                    PRIVATE BOARD_TYPE=\"${BOARD_TYPE}\"
                    )
