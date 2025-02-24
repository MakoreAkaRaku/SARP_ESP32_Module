#include "WiFiHandler.h"
#include "LedHandler.h"

#define MAX_RETRIES 8
static const char TAG[] = "WiFiHandler";
static int conn_retries = 0;
static const int CONNECTED_BIT = BIT0;
static struct WiFiCredential
{
    char SSID[32];  // "TP-Link_21C4" dummy data
    char PWD[64];   // "48997874" dummy data
} credential;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

void InitWiFi()
{
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
        ESP_LOGI(TAG, "Credentials found for SSID:[%s] and PWD:[%s]", wifi_conf.sta.ssid, wifi_conf.sta.password);
        strncpy(credential.SSID, (char *)wifi_conf.sta.ssid, sizeof(wifi_conf.sta.ssid));
        strncpy(credential.PWD, (char *)wifi_conf.sta.password, sizeof(wifi_conf.sta.password));
        hasCredentials = true;
    }
}

bool SwitchWiFi()
{
    static bool isOn = false;
    if (!isOn)
    {
        wifi_config_t conf = {
            .sta = {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Use WPA2-PSK
                .pmf_cfg = {
                    .capable = true,
                    .required = false
                }
            },
        };
        strncpy((char *)conf.sta.ssid, credential.SSID, sizeof(credential.PWD));
        strncpy((char *)conf.sta.password, "8741455"/*credential.PWD*/, sizeof(credential.PWD));
        esp_wifi_set_config(WIFI_IF_STA, &conf);
        ESP_ERROR_CHECK(esp_wifi_start());
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_stop());
    }

    return isOn = !isOn;
}

bool HasCredentialsSaved()
{
    return strlen(credential.PWD) != 0;
}

bool HasConnection()
{
    return CONNECTED_BIT == 1;
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
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        if(conn_retries < MAX_RETRIES) {
            ESP_LOGI(TAG, "Disconnected, retrying... [%d]",conn_retries++);       
            LEDEvent(WIFI_CONNECTING);
            esp_wifi_connect();
        }
        else {
            ESP_LOGI(TAG,"Max retries reached, starting to listen to BLE");
            conn_retries = 0;
            LEDEvent(SWAP_TO_BLE_SCAN);
            esp_wifi_stop();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG,"Got ip: "IPSTR,IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
        LEDEvent(WIFI_CONNECTED);
    }
    else if (event_base== WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "Connected, Waiting for DHCP protocol...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected, something happened.");
    }
}