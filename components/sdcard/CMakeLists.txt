
set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_SRCS "sdcard.c")
set(COMPONENT_PRIV_REQUIRES fatfs spiffs)

register_component()
