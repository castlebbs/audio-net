set(COMPONENT_ADD_INCLUDEDIRS ./include)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES audio_sal)

set(COMPONENT_SRCS ./audio_hal.c)

register_component()

#target_link_libraries(${COMPONENT_TARGET} INTERFACE "-L${CMAKE_CURRENT_LIST_DIR}/driver/zl38063/firmware")
#target_link_libraries(${COMPONENT_TARGET} INTERFACE firmware)
