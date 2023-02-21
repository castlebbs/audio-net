
set(COMPONENT_ADD_INCLUDEDIRS "button"
								"console")

set(COMPONENT_SRCS "button/button.c"
					"console/console.c")

set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES audio_sal console sdcard monitor)


register_component()
