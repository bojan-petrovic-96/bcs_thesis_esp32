idf_component_register(SRCS "gui.c" "./icons/settings_unit.c" "./icons/light_unit.c" "./icons/ac_unit.c"
                    INCLUDE_DIRS "include" "icons" ".."
                    REQUIRES "lvgl" "mqtt_config")
