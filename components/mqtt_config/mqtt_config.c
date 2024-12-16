#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "wifi_config.h"
#include "mqtt_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#define TAG "MQTT:"
#define BIT_0	( 1 << 0 )
#define BIT_4	( 1 << 4 )
extern gui_mqtt msg;

esp_mqtt_client_handle_t client;
char temperature[1];
static EventGroupHandle_t mqtt_event_handle;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            // esp_mqtt_client_disconnect(client);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            break;
        case MQTT_EVENT_PUBLISHED:
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (MQTT_ERROR_TYPE_TCP_TRANSPORT == event->error_handle->error_type) {
                ESP_LOGE(TAG, "Last error %s: 0x%x", "reported from esp-tls", \
                    event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last error %s: 0x%x", "reported from tls stack", \
                    event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last error %s: 0x%x", "captured as transport's socket errno",  \
                    event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", \
                    strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = CONFIG_MQTT_URL,
        .port = CONFIG_MQTT_PORT,
        .username = CONFIG_MQTT_UNAME,
        .password = CONFIG_MQTT_PASSWORD
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "Service started");
}

extern uint8_t is_connected;
void start_mqtt_service(void)
{
    ESP_LOGI(TAG, "Service starting...");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(connect_to_wifi());
	while(!is_connected){
		ESP_LOGI("", "\r\n");
	}
    mqtt_app_start();
}

void publish_value_mqtt(gui_mqtt *msg)
{
  char * value_str = itoa(msg->value, temperature, 10);
  esp_mqtt_client_publish(client, msg->topic, value_str, 0, 0, 0);
}

static void mqtt_pub(void *pvData)
{
    mqtt_event_handle = xEventGroupCreate();
    SemaphoreHandle_t countingSemaphore = xSemaphoreCreateCounting(10, 10);
    
    while(1)
    {
        xEventGroupWaitBits(
            mqtt_event_handle,
            BIT_4,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

        ESP_LOGI("", "Message value: %d, Message topic: %s.\r\n", msg.value, msg.topic);
        if (pdTRUE == xSemaphoreTake(countingSemaphore, 10)) {
            publish_value_mqtt(&msg);
            xSemaphoreGive(countingSemaphore);        
        }
        xEventGroupClearBits(mqtt_event_handle, BIT_4 );
    }
}

void mqtt_publish_task(const uint32_t usStackDepth,void * const pvParameters, \
    UBaseType_t uxPriority, TaskHandle_t * const pvCreatedTask, const BaseType_t xCoreID)
{
    xTaskCreatePinnedToCore(mqtt_pub, "mqtt_publish", usStackDepth, \
        pvParameters, uxPriority, pvCreatedTask, xCoreID);
}

EventGroupHandle_t get_mqtt_cfg_handle()
{
    return mqtt_event_handle;
}