#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
typedef struct gui_mqtt
{
    uint8_t value;
    char * topic;
}gui_mqtt;

void start_mqtt_service();
void publish_value_mqtt(gui_mqtt *msg);

EventGroupHandle_t get_mqtt_cfg_handle();
void mqtt_publish_task(const uint32_t usStackDepth,void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pvCreatedTask, const BaseType_t xCoreID);