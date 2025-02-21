#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

bool HasCredentialsSaved();

static bool hasCredentials = false;
bool HasConnection();

void InitWiFi();

/**
 * @brief Switches the Wifi to Start/Stop.
 * 
 * @return true if WiFi has been turned on.
 * @return false if not.
 */
bool SwitchWiFi();

static void WiFiEventHandler(void* arg, esp_event_base_t event_base, 
    int32_t event_id, void* event_data);