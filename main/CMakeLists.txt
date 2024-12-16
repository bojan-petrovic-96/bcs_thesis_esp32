# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES lvgl lvgl_esp32_drivers gui)
set(COMPONENT_PRIV_REQUIRES mqtt_config sntp_config)

set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
