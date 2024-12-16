#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

void refresh_lbl_time();
void gui_task(const uint32_t usStackDepth,void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pvCreatedTask, const BaseType_t xCoreID);