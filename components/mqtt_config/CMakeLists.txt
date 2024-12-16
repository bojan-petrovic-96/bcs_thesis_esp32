idf_component_register(SRCS "mqtt_config.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "mqtt" "wifi_config" "nvs_flash")
