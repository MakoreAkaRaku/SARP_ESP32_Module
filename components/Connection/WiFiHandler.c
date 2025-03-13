#include "WiFiHandler.h"
#include "LeScanner.h"
#include "LedHandler.h"

#define MAX_RETRIES 3
static const char TAG[] = "WiFiHandler";
static int con_retry = 0;
static const int WIFI_CONNECT_BIT = BIT0;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

void InitWiFi()
{
    ESP_LOGI(TAG, "InitWifi");
    esp_netif_init();
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiEventHandler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiEventHandler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_conf;
    if (ESP_OK == esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_conf))
    {
        // In case of first time, we set a dummyfied defaulted values.
        if (strlen((const char *)wifi_conf.sta.ssid) == 0)
        {
            SetCredentials((uint8_t *)"NAN", (uint8_t *)"NAN");
        }
        else
        {
            ESP_LOGI(TAG, "Credentials found for SSID:[%s] and PWD:[%s]", wifi_conf.sta.ssid, wifi_conf.sta.password);
        }
    }
}

bool SwitchWiFi()
{
    static bool isOn = false;
    if (!isOn)
        ESP_ERROR_CHECK(esp_wifi_start());
    else
        ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_LOGI(TAG, "WiFi switch: %d", !isOn);
    return isOn = !isOn;
}

void SetCredentials(const uint8_t *ssid, const uint8_t *pwd)
{
    wifi_config_t wifi_conf;

    wifi_config_t conf = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Use WPA2-PSK
            .pmf_cfg = {
                .capable = true,
                .required = false}},
    };
    strncpy((char *)conf.sta.ssid, (const char *)ssid, SSID_SIZE);
    strncpy((char *)conf.sta.password, (const char *)pwd, PWD_SIZE);
    esp_wifi_set_config(WIFI_IF_STA, &conf);
}

bool BlockUntilHasConnection()
{
    ESP_LOGI(TAG, "blocking thread");
    EventBits_t a = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "unblocking thread");
    return a == WIFI_CONNECT_BIT;
}

static void SwitchToLEScanCallback()
{
    SwitchWiFi();
    EnableBLE();
    StartScan();
}

static void WiFiEventHandler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    ESP_LOGI(TAG, "event number %" PRId32, event_id);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "event handler has started, trying to connect...");
        LEDEvent(WIFI_CONNECTING);
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECT_BIT);
        if (++con_retry > MAX_RETRIES)
        {
            ESP_LOGI(TAG, "Max retries reached, starting to listen to BLE");
            con_retry = 0;
            LEDEvent(SWITCH_TO_BLE_SCAN);
            xTaskCreate(SwitchToLEScanCallback, "switch_to_LEScan", 2096, NULL, 5, NULL);
        }
        else
        {
            ESP_LOGI(TAG, "Disconnected, retrying... [%d]", con_retry);
            LEDEvent(WIFI_CONNECTING);
            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        con_retry = 0;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECT_BIT);
        LEDEvent(WIFI_CONNECTED);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected, Waiting for DHCP protocol...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected, something happened.");
    }
}