/* Common functions for protocol examples, to establish Wi-Fi or Ethernet connection.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "esp_netif.h"


#define WIFI_INTERFACE get_example_netif()


esp_err_t connect_to_wifi(void);

esp_err_t disconnect_from_wifi(void);


esp_netif_t *get_wifi_netif(void);
esp_netif_t *get_wifi_netif_from_desc(const char *desc);

#ifdef __cplusplus
}
#endif