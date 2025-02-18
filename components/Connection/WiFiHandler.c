#include "WiFiHandler.h"

static const CONNECTED_BIT = BIT0;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

void WiFiInit()
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiEventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiEventHandler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_conf;
    if (ESP_OK == esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_conf))
    {
        hasCredentials = true;
    }
}

bool SwitchWiFi()
{
    static bool isOn = false;
    if (isOn)
        ESP_ERROR_CHECK(esp_wifi_stop());
    else
        ESP_ERROR_CHECK(esp_wifi_start());
    return isOn = !isOn;
}

bool HasCredentialsSaved()
{
    return hasCredentials;
}

bool HasConnection()
{
    // TODO:
    return false;
}

static void WiFiEventHandler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        // When esp_wifi_start() calls this once
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip: %s", ip4addr_ntoa(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    }
}