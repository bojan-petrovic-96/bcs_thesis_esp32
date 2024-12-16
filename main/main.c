#include "gui.h"
#include "sntp_config.h"
#include "mqtt_config.h"

#define SNTP_TASK_STACK_SIZE            1024 * 2
#define SNTP_TASK_PRIORITY              1
#define SNTP_TASK_CORE_AFFINITY         0

#define GUI_TASK_STACK_SIZE             1024 * 8
#define GUI_TASK_PIORITY                0
#define GUI_TASK_CORE_AFFINITY          1

#define MQTT_PUSLISH_TASK_SIZE          1024 * 2
#define MQTT_PUBLISH_TASK_PRIORITY      1
#define MQTT_PUBLISH_TASK_CORE_AFFINITY 0

void app_main(void)
{
    gui_task(GUI_TASK_STACK_SIZE, NULL, \
        GUI_TASK_PIORITY, NULL, GUI_TASK_CORE_AFFINITY);
    start_mqtt_service();
	mqtt_publish_task(MQTT_PUSLISH_TASK_SIZE, NULL, \
		MQTT_PUBLISH_TASK_PRIORITY, NULL, MQTT_PUBLISH_TASK_CORE_AFFINITY);
    sntp_task(SNTP_TASK_STACK_SIZE, NULL, \
        SNTP_TASK_PRIORITY, NULL, SNTP_TASK_CORE_AFFINITY);
        
}