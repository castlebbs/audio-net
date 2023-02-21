idf_component_register(SRCS "bt_app_av.c"
                            "bt_app_core.c"
                            "button_io.c"
                            "i2s_writer.c"
                            "i2s_reader.c"
                            "file_writer.c"
                            "file_reader.c"
                            "audio_dispatch.c"
                            "main.c"
                    INCLUDE_DIRS ".")

	    # Import the library, specifying a target name and the library path.
# The private dependencies of the library is also specified.
#add_prebuilt_library(prebuilt "libesp_processing.a")

# `main` calls a function from the library, so link it to `main`
#target_link_libraries(${COMPONENT_LIB} PRIVATE prebuilt)

