#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"

static const char TAG[] = "smartconfig_example";

bool HasCredentialsSaved();

static private bool hasCredentials = false;
static private bool hasConnection = false;
bool HasConnection();

void WiFiInit();

/**
 * @brief Switches the Wifi to Start/Stop.
 * 
 * @return true if WiFi has been turned on.
 * @return false if not.
 */
bool SwitchWiFi();

static void WiFiEventHandler(void* arg, esp_event_base_t event_base, 
    int32_t event_id, void* event_data);