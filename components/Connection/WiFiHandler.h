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

#define MAX_SSID_SIZE 16 // in Bytes
#define MAX_PWD_SIZE 16  // in Bytes
bool HasCredentialsSaved();

bool BlockUntilHasConnection();

/**
 * @brief The InitWiFi function initializes the Wi-Fi subsystem for an ESP32 device,
 * setting up the necessary network interfaces, event handlers, and default configurations.
 * It also checks for existing Wi-Fi credentials, logging them if found, or setting default
 * placeholder values if none are available.
 *
 */
void InitWiFi();

/**
 * @brief Switches the Wifi to Start/Stop.
 *
 * @return true if WiFi has been turned on.
 * @return false if not.
 */
bool SwitchWiFi();

void SetCredentials(const uint8_t *ssid, const uint8_t *pwd);

static void SwitchToLEScanCallback();

static void WiFiEventHandler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data);