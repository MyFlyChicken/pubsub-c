set(SOURCES 
src/pubsub.c
src/sync_zephyr.c
)

if(CONFIG_PS_QUEUE_BUCKET)
    list(APPEND SOURCES src/psqueue_b.c)
elseif(CONFIG_PS_QUEUE_LL)
    list(APPEND SOURCES src/psqueue_ll.c)
endif()

set(COMPONENT_ADD_INCLUDEDIRS "src")

