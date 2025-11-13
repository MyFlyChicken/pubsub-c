cmake_minimum_required(VERSION 3.20.0)

# PubSub-C Zephyr 端口: 根据 Kconfig 选择源文件与编译宏，并把库链接到 app
set(SOURCES
    src/pubsub.c
    src/sync_zephyr.c
)

if(CONFIG_PS_QUEUE_BUCKET)
    list(APPEND SOURCES src/psqueue_b.c)
elseif(CONFIG_PS_QUEUE_LL)
    list(APPEND SOURCES src/psqueue_ll.c)
endif()

# 使用常规静态库并链接到 app（应用模式，不使用 zephyr_library）
add_library(pubsubc STATIC ${SOURCES})
target_include_directories(pubsubc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(pubsubc PUBLIC zephyr_interface)

if(CONFIG_PS_SYNC_ZEPHYR)
    target_compile_definitions(pubsubc PUBLIC PS_SYNC_ZEPHYR PLATFORM_ZEPHYR)
endif()

if(CONFIG_PS_QUEUE_BUCKET)
    target_compile_definitions(pubsubc PUBLIC PS_QUEUE_CUSTOM PS_QUEUE_BUCKET)
elseif(CONFIG_PS_QUEUE_LL)
    target_compile_definitions(pubsubc PUBLIC PS_QUEUE_CUSTOM PS_QUEUE_LL)
endif()

target_link_libraries(app PRIVATE pubsubc)

# zephyr_library()
# zephyr_library_include_directories(src)
# zephyr_compile_definitions(PLATFORM_ZEPHYR)
# zephyr_library_sources_ifdef(CONFIG_PUBSUB_C_ZEPHYR src/pubsub.c src/sync_zephyr.c)
# zephyr_library_sources_ifdef(CONFIG_PS_QUEUE_BUCKET src/psqueue_b.c)
# zephyr_library_sources_ifdef(CONFIG_PS_QUEUE_LL src/psqueue_ll.c)
