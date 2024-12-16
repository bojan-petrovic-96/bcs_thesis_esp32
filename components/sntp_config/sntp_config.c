#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "sntp.h"
#include "gui.h"
#include "sntp_config.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"


time_t now = 0;
struct tm timeinfo = { 0 };

void time_sync_cb(struct timeval *tv)
{
    ESP_LOGI("SNTP: ", "Time synchronization completed");
}

static void sntp_initialize()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_init();

    time(&now);
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

}

static void refresh_label_time(void *pvData)
{
    static char strftime_buf[256];
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    refresh_lbl_time();
}

time_t *get_time_t()
{
    return &now;
}

struct tm *get_tm()
{
    return &timeinfo;
}

int32_t get_tm_year()
{
    return timeinfo.tm_year;
}

void sntp_cfg(void *pvParameter)
{
    sntp_initialize();
    const esp_timer_create_args_t lbl_time_timer_args = {
        .callback = &refresh_label_time,
        .name = "refresh_label"
    };
    esp_timer_handle_t lbl_timer_handle;

    ESP_ERROR_CHECK(esp_timer_create(&lbl_time_timer_args, &lbl_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lbl_timer_handle, 1000000));

    while(1)
    {
        if (SNTP_SYNC_STATUS_COMPLETED == sntp_get_sync_status())
        {
            time(&now);
            localtime_r(&now, &timeinfo);
            vTaskDelete(NULL);
        }
        ESP_LOGI("SNTP", "Waiting for system time to be set..");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void sntp_task(const uint32_t usStackDepth,void * const pvParameters, \
    UBaseType_t uxPriority, TaskHandle_t * const pvCreatedTask, const BaseType_t xCoreID)
{
    xTaskCreatePinnedToCore(sntp_cfg, "sntp", usStackDepth, pvParameters, \
        uxPriority, pvCreatedTask, xCoreID);
}
